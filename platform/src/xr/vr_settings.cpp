// The VR settings panel: a small panel beside the game's pause menu, drawn by
// the VR layer and worked with the pointer (aim the right controller, press
// A).  It holds settings of the VR presentation itself: the diorama
// distance, how far in front of the player Mario stands (while the giant
// screen is on, the slider sets the screen's distance instead), SpaceWarp
// ("smooth motion"), Super Resolution, FidelityFX CAS sharpening, the
// lowest render resolution, the giant screen (gameplay on a big virtual
// screen instead of the diorama) and its stereoscopic 3D (a picture for each
// eye, so the game has depth) with how deep it is.  A change applies at once (the paused scene
// behind the menu moves), and the settings file (petari_vr.ini) is updated
// when the menu closes.  In the headset the panel goes out as a compositor
// layer of its own (vr::uiLayer), sharp whatever the eye images' size.
//
// The panel is drawn on the CPU into a texture whenever something on it
// changes: rounded shapes with anti-aliased edges and text from the baked
// glyph atlas (ui_canvas.h).
#include <GLES3/gl32.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "port/heap_routing.h"
#include "port/port.h"
#include "ui_canvas.h"
#include "vr_renderer.h"

namespace {

using ui::Canvas;
using ui::Color;
using ui::kFontLarge;
using ui::kFontSmall;
using ui::rgb;

// ---------------------------------------------------------------------------
// The panel
// ---------------------------------------------------------------------------
const int kTexW = 640, kTexH = 1310;
const float kWidthM = 0.42f;  // metres; height from the texture's aspect
// Beside the pause menu's buttons, in the space right of them on the HUD
// panel (see kHudCenter in vr_game.cpp), a little in front of it and turned
// towards the player.
const xm::Vec3 kCenter{0.60f, -0.12f, -0.88f};

// The first row's slider: the diorama's distance, or while the giant screen
// is on, the screen's.
struct Range {
    float min, max, def, step;
};
const Range kDioramaRange{0.6f, 4.0f, 1.5f, 0.1f};
const Range kScreenRange{2.5f, 10.0f, 4.5f, 0.5f};
const Range& range() { return vr::giantScreen() ? kScreenRange : kDioramaRange; }
float distanceNow() { return vr::giantScreen() ? vr::screenDistance() : vr::dioramaDistance(); }

// Controls, in texture pixels (top-down rows).
enum Control {
    kNone, kReset, kMinus, kPlus, kSlider, kSpaceWarp, kSuperRes, kSharpen, kResMinus, kResPlus, kGiant, kStereo, kDepthMinus, kDepthPlus
};
const float kRowY = 236.0f;
const float kMinusX = 60.0f, kPlusX = 580.0f, kButtonR = 30.0f;
const float kTrackX0 = 116.0f, kTrackX1 = 524.0f;
// The SpaceWarp switch: a pill at the right of its row.
const float kSwitchY = 446.0f, kSwitchX0 = 520.0f, kSwitchX1 = 608.0f, kSwitchR = 22.0f;
// The Super Resolution switch, the row below, and the CAS switch below that.
const float kSwitch2Y = 572.0f;
const float kSwitch3Y = 698.0f;
// The lowest resolution: - value + at the right of the last row.
const float kResRowY = 824.0f, kResMinusX = 420.0f, kResPlusX = 580.0f, kResStep = 0.05f;
// The giant screen switch, and below it the last row: its stereoscopic 3D.
const float kSwitch4Y = 950.0f;
const float kSwitch5Y = 1076.0f;
// The 3D's depth: - value + like the lowest resolution's, the last row.
const float kDepthRowY = 1202.0f, kDepthStep = 0.25f;

std::atomic<int> sMenuSelecting{0};
std::atomic<int64_t> sMenuReportedAt{0};

std::string sIniPath;
bool sReady = false;
GLuint sTex = 0;
Canvas sStatic, sCanvas;
std::vector<uint32_t> sUpload;
xm::Vec3 sRight, sUp, sNormal;
xm::Mat4 sModel;

float sAlpha = 0.0f;
bool sShown = false;
int64_t sLastUpdateNs = 0;
Control sHover = kNone, sPressed = kNone;
bool sClickDown = false, sOwnsClick = false, sTick = false;
int64_t sPressNs = 0, sRepeatNs = 0;
bool sDirty = true, sUnsaved = false;
int64_t sChangedNs = 0;
bool sPointerOnPanel = false;
float sPointerPx = 0.0f, sPointerPy = 0.0f;  // where it is, texture pixels
float sShownScale = -1.0f;                   // render scale on the panel's picture

float heightM() { return kWidthM * kTexH / kTexW; }

void setupPose() {
    sNormal = xm::normalize(kCenter * -1.0f);  // facing the player's starting head position
    sRight = xm::normalize(xm::cross(xm::Vec3{0.0f, 1.0f, 0.0f}, sNormal));
    sUp = xm::cross(sNormal, sRight);
    xm::Mat4 m = xm::Mat4::identity();
    xm::Vec3 axes[3] = {sRight * kWidthM, sUp * heightM(), sNormal};
    for (int c = 0; c < 3; c++) {
        m.at(0, c) = axes[c].x;
        m.at(1, c) = axes[c].y;
        m.at(2, c) = axes[c].z;
    }
    m.at(0, 3) = kCenter.x;
    m.at(1, 3) = kCenter.y;
    m.at(2, 3) = kCenter.z;
    sModel = m;
}

// The parts that never change: background, title, captions.
void drawStatic() {
    Canvas& c = sStatic;
    c.init(kTexW, kTexH);
    c.roundRect(0, 0, kTexW, kTexH, 30.0f, rgb(78, 86, 106, 0.95f));
    c.roundRect(2, 2, kTexW - 2, kTexH - 2, 28.0f, rgb(22, 25, 33));
    c.text(32, 62, kFontLarge, "VR settings", rgb(255, 255, 255));
    c.text(kTrackX0, 306, kFontSmall, "Nearer", rgb(140, 147, 163));
    c.text(kTrackX1, 306, kFontSmall, "Farther", rgb(140, 147, 163), 2);
    c.roundRect(32, 396, kTexW - 32, 398, 1.0f, rgb(58, 63, 78));
    c.text(32, kSwitchY + 10, kFontSmall, "Smooth motion", rgb(196, 202, 216));
    c.text(32, 504, kFontSmall, "SpaceWarp: 120 frames a second", rgb(140, 147, 163));
    c.roundRect(32, 526, kTexW - 32, 528, 1.0f, rgb(58, 63, 78));
    c.text(32, kSwitch2Y + 10, kFontSmall, "Super resolution", rgb(196, 202, 216));
    c.text(32, 630, kFontSmall, "Sharper upscaling by the headset", rgb(140, 147, 163));
    c.roundRect(32, 652, kTexW - 32, 654, 1.0f, rgb(58, 63, 78));
    c.text(32, kSwitch3Y + 10, kFontSmall, "FidelityFX CAS", rgb(196, 202, 216));
    c.text(32, 756, kFontSmall, "Contrast adaptive sharpening", rgb(140, 147, 163));
    c.roundRect(32, 778, kTexW - 32, 780, 1.0f, rgb(58, 63, 78));
    c.text(32, kResRowY + 10, kFontSmall, "Lowest resolution", rgb(196, 202, 216));
    c.roundRect(32, 904, kTexW - 32, 906, 1.0f, rgb(58, 63, 78));
    c.text(32, kSwitch4Y + 10, kFontSmall, "Giant screen", rgb(196, 202, 216));
    c.text(32, 1008, kFontSmall, "Play on a big screen, no diorama", rgb(140, 147, 163));
    c.roundRect(32, 1030, kTexW - 32, 1032, 1.0f, rgb(58, 63, 78));
    c.roundRect(32, 1156, kTexW - 32, 1158, 1.0f, rgb(58, 63, 78));
}

float sliderX(float distance) { return kTrackX0 + (kTrackX1 - kTrackX0) * (distance - range().min) / (range().max - range().min); }

void drawPanel() {
    sCanvas.px = sStatic.px;
    Canvas& c = sCanvas;
    const Color accent = rgb(86, 160, 255);
    float d = distanceNow();
    bool giant = vr::giantScreen();
    c.text(32, 146, kFontSmall, giant ? "Screen distance" : "Diorama distance", rgb(196, 202, 216));
    c.text(32, 366, kFontSmall, giant ? "How far away the giant screen is" : "How far away Mario stands", rgb(140, 147, 163));

    // Reset
    Color resetFill = sPressed == kReset ? accent : sHover == kReset ? rgb(84, 93, 116) : rgb(44, 49, 62);
    c.roundRect(470, 24, 608, 70, 23.0f, resetFill);
    c.text(539, 56, kFontSmall, "Reset", rgb(230, 234, 242), 1);

    // Value
    char value[64];
    snprintf(value, sizeof(value), "%.1f m", d);
    c.text(608, 150, kFontLarge, value, rgb(255, 255, 255), 2);

    // Minus / plus buttons
    const float bar = 22.0f, thick = 5.0f;
    for (int i = 0; i < 2; i++) {
        Control ctl = i == 0 ? kMinus : kPlus;
        float x = i == 0 ? kMinusX : kPlusX;
        Color fill = sPressed == ctl ? accent : sHover == ctl ? rgb(84, 93, 116) : rgb(44, 49, 62);
        c.circle(x, kRowY, kButtonR, fill);
        c.roundRect(x - bar * 0.5f, kRowY - thick * 0.5f, x + bar * 0.5f, kRowY + thick * 0.5f, thick * 0.5f, rgb(240, 243, 248));
        if (ctl == kPlus) {
            c.roundRect(x - thick * 0.5f, kRowY - bar * 0.5f, x + thick * 0.5f, kRowY + bar * 0.5f, thick * 0.5f, rgb(240, 243, 248));
        }
    }

    // Slider: track, the part up to the knob, knob
    float kx = sliderX(d);
    c.roundRect(kTrackX0, kRowY - 6, kTrackX1, kRowY + 6, 6.0f, rgb(58, 63, 78));
    c.roundRect(kTrackX0, kRowY - 6, kx, kRowY + 6, 6.0f, accent);
    // The default distance, as a tick under the track.
    float dx = sliderX(range().def);
    c.roundRect(dx - 1.5f, kRowY + 14, dx + 1.5f, kRowY + 24, 1.5f, rgb(140, 147, 163));
    bool active = sHover == kSlider || sPressed == kSlider;
    if (active) {
        c.circle(kx, kRowY, 32.0f, rgb(86, 160, 255, 0.3f));
    }
    c.circle(kx, kRowY, active ? 22.0f : 19.0f, rgb(255, 255, 255));

    // On/off switches: SpaceWarp, Super Resolution, CAS, the giant screen
    // and its 3D (`live`: the switch has an effect as things are).
    auto drawSwitch = [&](Control ctl, float y, bool on, bool live = true) {
        bool swActive = sHover == ctl || sPressed == ctl;
        Color pill = on ? (live ? accent : rgb(52, 84, 128)) : swActive ? rgb(84, 93, 116) : rgb(58, 63, 78);
        c.roundRect(kSwitchX0, y - kSwitchR, kSwitchX1, y + kSwitchR, kSwitchR, pill);
        c.circle(on ? kSwitchX1 - kSwitchR : kSwitchX0 + kSwitchR, y, swActive ? 19.0f : 17.0f, live ? rgb(255, 255, 255) : rgb(150, 156, 170));
        c.text(kSwitchX0 - 16, y + 10, kFontSmall, on ? "On" : "Off", live ? rgb(230, 234, 242) : rgb(140, 147, 163), 2);
    };
    drawSwitch(kSpaceWarp, kSwitchY, vr::spaceWarp());
    drawSwitch(kSuperRes, kSwitch2Y, vr::superResolution());
    drawSwitch(kSharpen, kSwitch3Y, vr::sharpening());
    drawSwitch(kGiant, kSwitch4Y, vr::giantScreen());
    // The 3D belongs to the giant screen: dimmed while that is off.
    c.text(32, kSwitch5Y + 10, kFontSmall, "Stereoscopic 3D", giant ? rgb(196, 202, 216) : rgb(140, 147, 163));
    c.text(32, 1134, kFontSmall, giant ? "A picture for each eye" : "Only on the giant screen", rgb(140, 147, 163));
    drawSwitch(kStereo, kSwitch5Y, vr::stereoScreen(), giant);
    // Its depth: - value +, dimmed while there is no 3D.
    bool stereo = giant && vr::stereoScreen();
    c.text(32, kDepthRowY + 10, kFontSmall, "3D depth", stereo ? rgb(196, 202, 216) : rgb(140, 147, 163));
    c.text(32, 1260, kFontSmall, "More brings the world out of the screen", rgb(140, 147, 163));
    for (int i = 0; i < 2; i++) {
        Control ctl = i == 0 ? kDepthMinus : kDepthPlus;
        float x = i == 0 ? kResMinusX : kResPlusX;
        Color fill = sPressed == ctl ? accent : sHover == ctl ? rgb(84, 93, 116) : rgb(44, 49, 62);
        Color sign = stereo ? rgb(240, 243, 248) : rgb(140, 147, 163);
        c.circle(x, kDepthRowY, kButtonR, fill);
        c.roundRect(x - bar * 0.5f, kDepthRowY - thick * 0.5f, x + bar * 0.5f, kDepthRowY + thick * 0.5f, thick * 0.5f, sign);
        if (ctl == kDepthPlus) {
            c.roundRect(x - thick * 0.5f, kDepthRowY - bar * 0.5f, x + thick * 0.5f, kDepthRowY + bar * 0.5f, thick * 0.5f, sign);
        }
    }
    snprintf(value, sizeof(value), "%.2f", vr::stereoDepth());
    c.text((kResMinusX + kResPlusX) * 0.5f, kDepthRowY + 10, kFontSmall, value, stereo ? rgb(255, 255, 255) : rgb(140, 147, 163), 1);

    // Lowest resolution: - value +, and the scale the game renders at now.
    for (int i = 0; i < 2; i++) {
        Control ctl = i == 0 ? kResMinus : kResPlus;
        float x = i == 0 ? kResMinusX : kResPlusX;
        Color fill = sPressed == ctl ? accent : sHover == ctl ? rgb(84, 93, 116) : rgb(44, 49, 62);
        c.circle(x, kResRowY, kButtonR, fill);
        c.roundRect(x - bar * 0.5f, kResRowY - thick * 0.5f, x + bar * 0.5f, kResRowY + thick * 0.5f, thick * 0.5f, rgb(240, 243, 248));
        if (ctl == kResPlus) {
            c.roundRect(x - thick * 0.5f, kResRowY - bar * 0.5f, x + thick * 0.5f, kResRowY + bar * 0.5f, thick * 0.5f, rgb(240, 243, 248));
        }
    }
    snprintf(value, sizeof(value), "%.2f", vr::minResolution());
    c.text((kResMinusX + kResPlusX) * 0.5f, kResRowY + 10, kFontSmall, value, rgb(255, 255, 255), 1);
    sShownScale = roundf(vr::renderScale() * 100.0f) / 100.0f;
    snprintf(value, sizeof(value), "Now %.2f; 1.00 is Meta's standard", sShownScale);
    c.text(32, 882, kFontSmall, value, rgb(140, 147, 163));
}

void upload() {
    // Rows bottom up for GL.
    sUpload.resize((size_t)kTexW * kTexH);
    for (int y = 0; y < kTexH; y++) {
        memcpy(&sUpload[(size_t)y * kTexW], &sCanvas.px[(size_t)(kTexH - 1 - y) * kTexW], kTexW * 4);
    }
    glBindTexture(GL_TEXTURE_2D, sTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kTexW, kTexH, GL_RGBA, GL_UNSIGNED_BYTE, sUpload.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

// Writes `key = value` into the settings file, replacing the key's line or
// appending one; the other lines (comments included) stay as they are.
void saveSettingText(const std::string& path, const char* key, const std::string& value);

void saveSetting(const std::string& path, const char* key, float value) {
    char text[64];
    snprintf(text, sizeof(text), "%g", value);
    saveSettingText(path, key, text);
}

void saveSettingText(const std::string& path, const char* key, const std::string& value) {
    std::string out;
    bool replaced = false;
    if (FILE* f = fopen(path.c_str(), "r")) {
        char line[512];
        while (fgets(line, sizeof(line), f)) {
            const char* p = line;
            while (*p == ' ' || *p == '\t') p++;
            size_t n = strlen(key);
            const char* after = p + n;
            while (*after == ' ' || *after == '\t') after++;
            if (!replaced && strncmp(p, key, n) == 0 && *after == '=') {
                out += std::string(key) + " = " + value + "\n";
                replaced = true;
            } else {
                out += line;
                if (!out.empty() && out.back() != '\n') out += '\n';
            }
        }
        fclose(f);
    } else {
        out = "# GalaxyQuest VR settings (see docs/CONTROLS.md)\n";
    }
    if (!replaced) {
        out += std::string(key) + " = " + value + "\n";
    }
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) {
        port_log("vr: cannot write %s", tmp.c_str());
        return;
    }
    bool ok = fwrite(out.data(), 1, out.size(), f) == out.size();
    ok = fclose(f) == 0 && ok;
    if (!ok || rename(tmp.c_str(), path.c_str()) != 0) {
        port_log("vr: cannot write %s", path.c_str());
        return;
    }
    port_log("vr: %s: %s = %s", path.c_str(), key, value.c_str());
}

void save() {
    sUnsaved = false;
    if (sIniPath.empty()) return;
    std::string path = sIniPath;
    float distance = vr::dioramaDistance();
    float screen = vr::screenDistance();
    float spaceWarp = vr::spaceWarp() ? 1.0f : 0.0f;
    float superRes = vr::superResolution() ? 1.0f : 0.0f;
    float sharpen = vr::sharpening() ? 1.0f : 0.0f;
    float minRes = vr::minResolution();
    float giant = vr::giantScreen() ? 1.0f : 0.0f;
    float stereo = vr::stereoScreen() ? 1.0f : 0.0f;
    float depth = vr::stereoDepth();
    PortHostAllocScope hostAlloc;
    std::thread([path, distance, screen, spaceWarp, superRes, sharpen, minRes, giant, stereo, depth] {
        PortHostAllocScope scope;
        saveSetting(path, "diorama_distance", distance);
        saveSetting(path, "screen_distance", screen);
        saveSetting(path, "space_warp", spaceWarp);
        saveSetting(path, "super_resolution", superRes);
        saveSetting(path, "sharpening", sharpen);
        saveSetting(path, "min_resolution", minRes);
        saveSetting(path, "giant_screen", giant);
        saveSetting(path, "stereo_screen", stereo);
        saveSetting(path, "stereo_depth", depth);
    }).detach();
}

}  // namespace

namespace vr {

void saveGamePath(const std::string& folder) {
    if (sIniPath.empty()) return;
    std::string path = sIniPath;
    PortHostAllocScope hostAlloc;
    std::thread([path, folder] {
        PortHostAllocScope scope;
        saveSettingText(path, "game_path", folder);
    }).detach();
}

}  // namespace vr

namespace {

void setDistance(float d) {
    const Range& r = range();
    d = fminf(r.max, fmaxf(r.min, roundf(d / r.step) * r.step));
    if (fabsf(d - distanceNow()) < 0.001f) return;
    if (vr::giantScreen()) {
        port_log("vr settings: screen distance %.1f m", d);
        vr::setScreenDistance(d);
    } else {
        port_log("vr settings: diorama distance %.1f m", d);
        vr::setDioramaDistance(d);
    }
    sDirty = true;
    sUnsaved = true;
    sChangedNs = port_host_time_ns();
}

const char* controlName(Control c) {
    static const char* const kNames[] = {"nothing",        "Reset", "-", "+", "the slider", "the SpaceWarp switch", "the Super Resolution switch",
                                         "the CAS switch", "resolution -", "resolution +", "the giant screen switch",
                                         "the stereoscopic 3D switch", "3D depth -", "3D depth +"};
    return kNames[c];
}

Control controlAt(float x, float y) {
    auto near = [](float x, float y, float cx, float cy, float r) { return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r; };
    if (x >= 462 && x <= 616 && y >= 16 && y <= 78) return kReset;
    if (near(x, y, kMinusX, kRowY, kButtonR + 10.0f)) return kMinus;
    if (near(x, y, kPlusX, kRowY, kButtonR + 10.0f)) return kPlus;
    if (x >= kTrackX0 - 16.0f && x <= kTrackX1 + 16.0f && y >= kRowY - 34.0f && y <= kRowY + 34.0f) return kSlider;
    if (x >= 24.0f && x <= kTexW - 16.0f && y >= kSwitchY - 36.0f && y <= kSwitchY + 36.0f) return kSpaceWarp;  // the whole row
    if (x >= 24.0f && x <= kTexW - 16.0f && y >= kSwitch2Y - 36.0f && y <= kSwitch2Y + 36.0f) return kSuperRes;
    if (x >= 24.0f && x <= kTexW - 16.0f && y >= kSwitch3Y - 36.0f && y <= kSwitch3Y + 36.0f) return kSharpen;
    if (near(x, y, kResMinusX, kResRowY, kButtonR + 10.0f)) return kResMinus;
    if (near(x, y, kResPlusX, kResRowY, kButtonR + 10.0f)) return kResPlus;
    if (x >= 24.0f && x <= kTexW - 16.0f && y >= kSwitch4Y - 36.0f && y <= kSwitch4Y + 36.0f) return kGiant;
    if (x >= 24.0f && x <= kTexW - 16.0f && y >= kSwitch5Y - 36.0f && y <= kSwitch5Y + 36.0f) return kStereo;
    if (near(x, y, kResMinusX, kDepthRowY, kButtonR + 10.0f)) return kDepthMinus;
    if (near(x, y, kResPlusX, kDepthRowY, kButtonR + 10.0f)) return kDepthPlus;
    return kNone;
}

}  // namespace

extern "C" void port_vr_pause_menu(int selecting) {
    sMenuSelecting.store(selecting);
    sMenuReportedAt.store(port_host_time_ns());
}

namespace vr {

void settingsInit(const char* iniPath) {
    sIniPath = iniPath ? iniPath : "";
    ui::initFonts();
    setupPose();
    drawStatic();
    sCanvas.init(kTexW, kTexH);
    glGenTextures(1, &sTex);
    glBindTexture(GL_TEXTURE_2D, sTex);
    int levels = 1 + (int)floorf(log2f((float)kTexW));
    glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, kTexW, kTexH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    sReady = true;
}

float settingsPointer(xm::Vec3 origin, xm::Vec3 dir, bool clickDown) {
    int64_t now = port_host_time_ns();
    float dt = sLastUpdateNs ? fminf(0.1f, (now - sLastUpdateNs) / 1e9f) : 0.0f;
    sLastUpdateNs = now;
    // Shown while the pause menu takes input (reported each game frame).
    bool wanted = sReady && sMenuSelecting.load() && now - sMenuReportedAt.load() < 250000000;
    sAlpha = wanted ? fminf(1.0f, sAlpha + dt / 0.15f) : fmaxf(0.0f, sAlpha - dt / 0.12f);
    if (sShown && !wanted && sUnsaved) {
        save();  // the menu closed
    } else if (sUnsaved && now - sChangedNs > 3000000000ll) {
        save();  // or three seconds after the last change, in case it never does
    }
    if (wanted != sShown) {
        port_log("vr settings: panel %s", wanted ? "shown" : "hidden");
    }
    sShown = wanted;

    float hitT = 0.0f;
    float px = 0.0f, py = 0.0f;
    if (wanted) {
        float dn = xm::dot(dir, sNormal);
        if (dn < -1e-4f) {
            float t = xm::dot(kCenter - origin, sNormal) / dn;
            xm::Vec3 p = origin + dir * t - kCenter;
            float u = xm::dot(p, sRight) / kWidthM + 0.5f, v = xm::dot(p, sUp) / heightM() + 0.5f;
            if (t > 0.0f && u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
                hitT = t;
                px = u * kTexW;
                py = (1.0f - v) * kTexH;
            }
        }
    }
    Control over = hitT > 0.0f ? controlAt(px, py) : kNone;
    sPointerPx = px;
    sPointerPy = py;
    // The render scale shown on the panel follows the dynamic resolution.
    if (wanted && fabsf(roundf(renderScale() * 100.0f) / 100.0f - sShownScale) > 0.001f) {
        sDirty = true;
    }
    if (over != sHover) {
        sHover = over;
        sDirty = true;
        if (over != kNone) sTick = true;
    }
    bool onPanel = hitT > 0.0f;
    if (onPanel != sPointerOnPanel) {
        sPointerOnPanel = onPanel;
        port_log("vr settings: pointer %s the panel", onPanel ? "on" : "off");
    }
    // A click (A or the trigger) made on the panel belongs to it until
    // released; one that started elsewhere stays the game's.
    if (clickDown && !sClickDown && hitT > 0.0f) {
        port_log("vr settings: click on %s", controlName(over));
        sOwnsClick = true;
        sPressed = over;
        sPressNs = sRepeatNs = now;
        float d = distanceNow();
        if (over == kMinus) setDistance(d - range().step);
        if (over == kPlus) setDistance(d + range().step);
        if (over == kReset) setDistance(range().def);
        if (over == kSpaceWarp) {
            setSpaceWarp(!spaceWarp());
            port_log("vr settings: SpaceWarp %s", spaceWarp() ? "on" : "off");
            sUnsaved = true;
            sChangedNs = now;
        }
        if (over == kSuperRes) {
            setSuperResolution(!superResolution());
            port_log("vr settings: Super Resolution %s", superResolution() ? "on" : "off");
            sUnsaved = true;
            sChangedNs = now;
        }
        if (over == kSharpen) {
            setSharpening(!sharpening());
            port_log("vr settings: FidelityFX CAS %s", sharpening() ? "on" : "off");
            sUnsaved = true;
            sChangedNs = now;
        }
        if (over == kGiant) {
            setGiantScreen(!giantScreen());
            port_log("vr settings: giant screen %s", giantScreen() ? "on" : "off");
            sUnsaved = true;
            sChangedNs = now;
        }
        if (over == kStereo) {
            setStereoScreen(!stereoScreen());
            port_log("vr settings: stereoscopic 3D %s%s", stereoScreen() ? "on" : "off", giantScreen() ? "" : " (for when the giant screen is on)");
            sUnsaved = true;
            sChangedNs = now;
        }
        if (over == kResMinus || over == kResPlus) {
            setMinResolution(minResolution() + (over == kResPlus ? kResStep : -kResStep));
            port_log("vr settings: lowest resolution %.2f", minResolution());
            sUnsaved = true;
            sChangedNs = now;
        }
        if (over == kDepthMinus || over == kDepthPlus) {
            setStereoDepth(stereoDepth() + (over == kDepthPlus ? kDepthStep : -kDepthStep));
            port_log("vr settings: 3D depth %.2f", stereoDepth());
            sUnsaved = true;
            sChangedNs = now;
        }
        sDirty = true;
    }
    if (sOwnsClick && clickDown) {
        if (sPressed == kSlider && hitT > 0.0f) {
            setDistance(range().min + (range().max - range().min) * (px - kTrackX0) / (kTrackX1 - kTrackX0));
        }
        // Held on - or +: repeats after a moment.
        if ((sPressed == kMinus || sPressed == kPlus) && over == sPressed && now - sPressNs > 450000000 && now - sRepeatNs > 90000000) {
            sRepeatNs = now;
            setDistance(distanceNow() + (sPressed == kPlus ? range().step : -range().step));
        }
        if ((sPressed == kResMinus || sPressed == kResPlus) && over == sPressed && now - sPressNs > 450000000 && now - sRepeatNs > 150000000) {
            sRepeatNs = now;
            setMinResolution(minResolution() + (sPressed == kResPlus ? kResStep : -kResStep));
            sDirty = true;
            sUnsaved = true;
            sChangedNs = now;
        }
        if ((sPressed == kDepthMinus || sPressed == kDepthPlus) && over == sPressed && now - sPressNs > 450000000 && now - sRepeatNs > 250000000) {
            sRepeatNs = now;
            setStereoDepth(stereoDepth() + (sPressed == kDepthPlus ? kDepthStep : -kDepthStep));
            sDirty = true;
            sUnsaved = true;
            sChangedNs = now;
        }
    }
    if (!clickDown) {
        if (sPressed != kNone) sDirty = true;
        sOwnsClick = false;
        sPressed = kNone;
    }
    sClickDown = clickDown;
    return hitT;
}

bool settingsOwnsClick() { return sOwnsClick; }

bool settingsShown() { return sReady && sAlpha > 0.0f; }

void settingsLayerSize(int* width, int* height) {
    *width = kTexW;
    *height = kTexH;
}

UiLayer settingsLayer() {
    UiLayer l;
    l.visible = settingsShown();
    l.changed = l.visible;  // a small image: redrawn each frame it is up (fade, reticle)
    xm::Mat4 basis = xm::Mat4::identity();
    xm::Vec3 axes[3] = {sRight, sUp, sNormal};
    for (int c = 0; c < 3; c++) {
        basis.at(0, c) = axes[c].x;
        basis.at(1, c) = axes[c].y;
        basis.at(2, c) = axes[c].z;
    }
    l.orientation = xm::quatFromMatrix(basis);
    l.position = kCenter;
    l.width = kWidthM;
    l.height = heightM();
    return l;
}

void settingsDrawLayer() {
    if (!sReady) return;
    if (sDirty) {
        drawPanel();
        upload();
        sDirty = false;
    }
    drawOverlayQuad(sTex, xm::scale(2.0f), sAlpha);
    // The laser's reticle is in the eye images, under this layer: the
    // panel shows where it points itself.
    if (sPointerOnPanel) {
        const float r = 13.0f;
        drawReticle2d(sPointerPx / kTexW * 2.0f - 1.0f, 1.0f - sPointerPy / kTexH * 2.0f, r / kTexW * 2.0f, r / kTexH * 2.0f);
    }
}

bool settingsTakeTick() {
    bool t = sTick;
    sTick = false;
    return t;
}

void settingsDraw(const xm::Mat4& viewProj) {
    if (!sReady || sAlpha <= 0.0f) return;
    if (sDirty) {
        drawPanel();
        upload();
        sDirty = false;
    }
    drawOverlayQuad(sTex, viewProj * sModel, sAlpha);
}

}  // namespace vr

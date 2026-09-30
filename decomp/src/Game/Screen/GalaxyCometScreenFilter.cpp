#include "Game/Screen/GalaxyCometScreenFilter.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

GalaxyCometScreenFilter::GalaxyCometScreenFilter() : LayoutActor("\x83\x52\x83\x81\x83\x62\x83\x67\x97\x70\x89\xe6\x96\xca\x93\x68\x82\xe8\x92\xd7\x82\xb5", true), _20(true) {
    MR::connectToScene(this, MR::MovementType_Layout, MR::CalcAnimType_Layout, MR::DrawBufferType_None, MR::DrawType_CometScreenFilter);
    initLayoutManager("CometScreenFilter", 1);
    appear();
}

void GalaxyCometScreenFilter::setCometType(const char* pCometName) {
    MR::setCometAnimFromId(this, MR::getCometNameIdFromString(pCometName), 0);
}

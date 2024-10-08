#include "Serialization/GLTFImport.h"
#include "Serialization/GLTFExport.h"
#include "Settings/Settings.h"
#include "zstr.hpp"

DLLEXPORT bool OptimizeAnimation(const char* filePath, const char* skeletonPath, int level, bool additive)
{
	uint8_t shortLevel = static_cast<uint8_t>(std::clamp(level, 0, 255));

	Settings::SetFaceMorphs({
		"browLowererL",
		"browLowererR",
		"cheekPuffL",
		"cheekPuffR",
		"cheekRaiseL",
		"cheekRaiseR",
		"cheekSuckL",
		"cheekSuckR",
		"chinRaise",
		"chinRaiseUpperlipTweak",
		"c_eyeDown_eyeClosedL",
		"c_eyeDown_eyeClosedR",
		"c_eyeLeft_eyeClosedL",
		"c_eyeLeft_eyeClosedR",
		"c_eyeRight_eyeClosedL",
		"c_eyeRight_eyeClosedR",
		"c_eyeUp_eyeClosedL",
		"c_eyeUp_eyeClosedR",
		"c_eyesClosed50L",
		"c_eyesClosed50R",
		"c_jawDrop",
		"c_squintL_cheekRaiserL",
		"c_squintR_cheekRaiserR",
		"dimplerL",
		"dimplerR",
		"eyeClosedL",
		"eyeClosedR",
		"eyeDown",
		"eyeLeft",
		"eyeOpenL",
		"eyeOpenR",
		"eyeRight",
		"eyeUp",
		"innerBrowRaiseL",
		"innerBrowRaiseR",
		"jawClench",
		"jawLeft",
		"jawOpen",
		"jawRight",
		"jawThrust",
		"lidTightenerL",
		"lidTightenerR",
		"lipCornerDepressL",
		"lipCornerDepressR",
		"lipCornerInL",
		"lipCornerInR",
		"lipCornerPullL",
		"lipCornerPullR",
		"lipPress",
		"lipPucker",
		"lipStretchL",
		"lipStretchR",
		"lipTighten",
		"lipZipperL",
		"lipZipperR",
		"lowerLipDepressL",
		"lowerLipDepressR",
		"lowerLipFunnel",
		"lowerLipPuff",
		"lowerLipSuck",
		"lowerLipThickness",
		"lowerLipUpL",
		"lowerLipUpR",
		"nasolabialFurrowL",
		"nasolabialFurrowR",
		"neckFlexL",
		"neckFlexR",
		"noseDepressor",
		"noseWrinkleL",
		"noseWrinkleR",
		"nostrilCompressor",
		"nostrilDilator",
		"outerBrowRaiseL",
		"outerBrowRaiseR",
		"sharpLipPullL",
		"sharpLipPullR",
		"squintL",
		"squintR",
		"swallow",
		"upperLipDownL",
		"upperLipDownR",
		"upperLipFunnel",
		"upperLipPuff",
		"upperLipRaiseL",
		"upperLipRaiseR",
		"upperLipSuck",
		"upperLipThickness",
		"tongueCurlDown",
		"tongueCurlUp",
		"tongueDown",
		"tongueUp",
		"tongueIn",
		"tongueOut",
		"tongueLeft",
		"tongueRight",
		"tongueThick",
		"tongueThinner",
		"LookDown",
		"LookUp",
		"LookRight",
		"LookLeft",
		"Hat",
		"HideEar",
		"Mask"
	});

	auto skeleFile = Serialization::GLTFImport::LoadGLTF(skeletonPath);
	if (!skeleFile) {
		return false;
	}

	auto skeleData = Serialization::GLTFImport::BuildSkeleton(skeleFile.get());
	if (!skeleData || !skeleData->skeleton) {
		return false;
	}

	skeleFile.reset();

	auto baseFile = Serialization::GLTFImport::LoadGLTF(filePath);
	if (!baseFile || baseFile->asset.animations.empty()) {
		return false;
	}

	auto rawAnim = Serialization::GLTFImport::CreateRawAnimation(baseFile.get(), &baseFile->asset.animations[0], skeleData->skeleton.get());
	if (!rawAnim) {
		return false;
	}

	baseFile.reset();

	if (additive) {
		ozz::animation::offline::AdditiveAnimationBuilder addBuilder;
		auto addResult = ozz::make_unique<ozz::animation::offline::RawAnimation>();
		if (!addBuilder(*rawAnim->data, ozz::make_span(skeleData->restPose), addResult.get())) {
			return false;
		}
		rawAnim->data = std::move(addResult);
	}

	auto optimizedAsset = Serialization::GLTFExport::CreateOptimizedAsset(rawAnim.get(), skeleData->skeleton.get(), shortLevel);

	try {
		zstr::ofstream file(filePath, std::ios::binary);
		file.write(reinterpret_cast<char*>(optimizedAsset.data()), optimizedAsset.size());
	} catch (const std::exception&) {
		return false;
	}

	return true;
}
#pragma once

namespace RE
{
	class NiCamera : public NiAVObject
	{
	public:
		NiPoint3 WorldPtToScreenPt3(const NiPoint3& worldPt)
		{
			const auto MatrixVectorMultiply = [](const float matrix[4][4], const NiPoint4& vector) {
				NiPoint4 result;

				for (int i = 0; i < 4; ++i) {
					for (int j = 0; j < 4; ++j) {
						result[i] += matrix[i][j] * vector[j];
					}
				}

				return result;
			};

			NiPoint4 cameraPoint = MatrixVectorMultiply(worldToCam6, { worldPt.x, worldPt.y, worldPt.z, 1.0f });

			float ndcX = cameraPoint[0] / cameraPoint[3];
			float ndcY = cameraPoint[1] / cameraPoint[3];
			float ndcZ = cameraPoint[2] / cameraPoint[3];

			return {
				(ndcX + 1.0f) * 0.5f,
				(1.0f - ndcY) * 0.5f,
				ndcZ
			};
		}

		float worldToCam[4][4];
		float worldToCam2[4][4];
		float worldToCam3[4][4];
		float worldToCam4[4][4];
		float worldToCam5[4][4];
		float worldToCam6[4][4];
		float worldToCam7[4][4];
		float worldToCam8[4][4];
		float worldToCam9[4][4];
	};
}
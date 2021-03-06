#include "stdafx.h"
#include "Geometry.hpp"

using namespace osg;

namespace Geometry
{
	const double g_epsilon = 0.00001;
	const double g_deflection = 0.5;
	const double g_defaultIncAngle = 10.0 * M_PI / 180.0;

	osg::ref_ptr<osg::Geometry> BuildCircularTorus(const osg::Vec3 &center, const osg::Vec3 &startPnt, const osg::Vec3 &normal,
		double radius, double angle, const osg::Vec4 &color, bool topVis /*= true*/, bool bottomVis /*= true*/)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		bool isFull = equivalent(angle, 2 * M_PI, g_epsilon);
		if (isFull)
		{
			angle = 2 * M_PI;
		}

		Vec3 mainVec = startPnt - center;
		double mainRadius = mainVec.length() + radius;
		double mainIncAng = 0.0;
		if (g_deflection > mainRadius)
			mainIncAng = angle / 4.0;
		else
			mainIncAng = 2 * acos((mainRadius - g_deflection) / mainRadius);
		int mainCount = (int)ceil(angle / mainIncAng);
		mainIncAng = angle / mainCount;
		Quat mainQuat(mainIncAng, normal);

		double subIncAng = 0.0;
		if (g_deflection > radius)
			subIncAng = M_PI_2;
		else
			subIncAng = 2.0 * acos((radius - g_deflection) / radius);
		int subCount = (int)ceil(2 * M_PI / subIncAng);
		subIncAng = 2 * M_PI / subCount;
		Vec3 subNormal = normal ^ mainVec;
		subNormal.normalize();
		Quat subQuat(subIncAng, subNormal);

		vector<Vec3> prevCircArr;
		Vec3 subVec = mainVec;
		subVec.normalize();
		subVec *= radius;
		for (int i = 0; i < subCount; ++i)
		{
			prevCircArr.push_back(startPnt + subVec);
			subVec = subQuat * subVec;
		}
		prevCircArr.push_back(prevCircArr.front());

		if (!isFull && bottomVis)
		{
			Vec3 bottomNormal = -subNormal;
			const GLint first = vertexArr->size();
			vertexArr->push_back(startPnt);
			normalArr->push_back(bottomNormal);
			for (int i = 0; i < subCount; ++i)
			{
				vertexArr->push_back(prevCircArr[i]);
				normalArr->push_back(bottomNormal);
			}
			vertexArr->push_back(prevCircArr[0]);
			normalArr->push_back(bottomNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		vector<Vec3> vecArr;
		const size_t prevCircArrCount = prevCircArr.size();
		for (size_t i = 0; i < prevCircArrCount; ++i)
		{
			vecArr.push_back(prevCircArr[i] - center);
		}
		Vec3 prevSubCenterVec = startPnt - center;

		vector<Vec3> postCircArr;
		Vec3 postSubCenterPnt;
		Vec3 topNormal;
		if (isFull)
		{
			postCircArr = prevCircArr;
			postSubCenterPnt = startPnt;
		}
		else
		{
			Quat fullQuat(angle, normal);
			for (size_t i = 0; i < prevCircArrCount; ++i)
			{
				postCircArr.push_back(center + fullQuat * vecArr[i]);
			}
			postSubCenterPnt = center + fullQuat * prevSubCenterVec;
			topNormal = fullQuat * subNormal;
		}

		for (int i = 0; i < mainCount - 1; ++i)
		{
			vector<Vec3> circArr;
			const size_t vecArrCount = vecArr.size();
			for (size_t j = 0; j < vecArrCount; ++j)
			{
				vecArr[j] = mainQuat * vecArr[j];
				circArr.push_back(center + vecArr[j]);
			}
			Vec3 subCenterVec = mainQuat * prevSubCenterVec;

			GLint first = vertexArr->size();
			for (size_t j = 0; j < vecArrCount - 1; ++j)
			{
				vertexArr->push_back(prevCircArr[j]);
				normalArr->push_back(prevCircArr[j] - (center + prevSubCenterVec));
				normalArr->back().normalize();

				vertexArr->push_back(circArr[j]);
				normalArr->push_back(circArr[j] - (center + subCenterVec));
				normalArr->back().normalize();

				vertexArr->push_back(prevCircArr[j + 1]);
				normalArr->push_back(prevCircArr[j + 1] - (center + prevSubCenterVec));
				normalArr->back().normalize();

				vertexArr->push_back(circArr[j + 1]);
				normalArr->push_back(circArr[j + 1] - (center + subCenterVec));
				normalArr->back().normalize();
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

			prevCircArr.swap(circArr);
			prevSubCenterVec = subCenterVec;
		}

		// 最后一环
		const size_t vecArrCount = vecArr.size();
		GLint first = vertexArr->size();
		for (size_t j = 0; j < vecArrCount - 1; ++j)
		{
			vertexArr->push_back(prevCircArr[j]);
			normalArr->push_back(prevCircArr[j] - (center + prevSubCenterVec));
			normalArr->back().normalize();

			vertexArr->push_back(postCircArr[j]);
			normalArr->push_back(postCircArr[j] - postSubCenterPnt);
			normalArr->back().normalize();

			vertexArr->push_back(prevCircArr[j + 1]);
			normalArr->push_back(prevCircArr[j + 1] - (center + prevSubCenterVec));
			normalArr->back().normalize();

			vertexArr->push_back(postCircArr[j + 1]);
			normalArr->push_back(postCircArr[j + 1] - postSubCenterPnt);
			normalArr->back().normalize();
		}
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		if (!isFull && topVis)
		{
			topNormal.normalize();
			const GLint first = vertexArr->size();
			vertexArr->push_back(postSubCenterPnt);
			normalArr->push_back(topNormal);
			for (int i = 0; i < subCount; ++i)
			{
				vertexArr->push_back(postCircArr[i]);
				normalArr->push_back(topNormal);
			}
			vertexArr->push_back(postCircArr[0]);
			normalArr->push_back(topNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildCircularTorus(const osg::Vec3 &center, const osg::Vec3 &startPnt, const osg::Vec3 &normal,
		double startRadius, double endRadius, double angle, const osg::Vec4 &color, bool topVis /*= true*/, bool bottomVis /*= true*/)
	{
		if (equivalent(startRadius, endRadius, g_epsilon))
		{
			return BuildCircularTorus(center, startPnt, normal, startRadius, angle, color, topVis, bottomVis);
		}

		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		bool isFull = equivalent(angle, 2 * M_PI, g_epsilon);
		if (isFull)
		{
			angle = 2 * M_PI;
		}

		int mainCount = (int)ceil(angle / g_defaultIncAngle);
		double mainIncAngle = angle / mainCount;

		double subIncAngle = g_defaultIncAngle;
		int subCount = (int)(2.0 * M_PI / subIncAngle);

		osg::Vec3 mainVec = startPnt - center;
		osg::Quat mainQuat(mainIncAngle, normal);

		osg::Vec3 torusNormal = mainVec;
		torusNormal.normalize();

		// 第一圈
		osg::Vec3 faceNormal = torusNormal ^ normal;
		osg::Quat torusQuat(subIncAngle, faceNormal);
		osg::Vec3 subVec = torusNormal;
		double subRadius = startRadius;
		osg::Vec3 subCenter = startPnt;
		osg::Vec3 tmpSubVec = subVec * subRadius;
		for (int i = 0; i < subCount; ++i)
		{
			vertexArr->push_back(subCenter + tmpSubVec);
			tmpSubVec = torusQuat * tmpSubVec;
		}

		// 中间
		osg::DrawElementsUShort *pDrawEle = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0);
		pDrawEle->push_back(subCount - 1);
		double factor = (endRadius - startRadius) / mainCount;
		for (int i = 1; i < mainCount; ++i)
		{
			faceNormal = mainQuat * faceNormal;
			mainVec = mainQuat * mainVec;
			subVec = mainQuat * subVec;
			subCenter = center + mainVec;
			torusQuat.makeRotate(subIncAngle, faceNormal);
			subRadius += factor;

			tmpSubVec = subVec * subRadius;
			osg::Vec3 tangNormal = normal;
			for (int j = 0; j < subCount; ++j)
			{
				vertexArr->push_back(subCenter + tmpSubVec);
				tmpSubVec = torusQuat * tmpSubVec;

				int size = vertexArr->size();
				normalArr->push_back(tangNormal ^ ((*vertexArr)[size - subCount - 1] - vertexArr->back()));
				normalArr->back().normalize();
				tangNormal = torusQuat * tangNormal;

				pDrawEle->push_back(size - subCount - 1);
				pDrawEle->push_back(size - 1);
			}
		}

		// 最后一圈
		osg::Quat fullQuat(angle, normal);
		faceNormal = fullQuat * (torusNormal ^ normal);
		mainVec = fullQuat * (startPnt - center);
		subVec = fullQuat * (torusNormal * endRadius);
		subCenter = center + mainVec;

		torusQuat.makeRotate(subIncAngle, faceNormal);
		tmpSubVec = subVec;
		osg::Vec3 tangNormal = normal;
		for (int j = 0; j < subCount; ++j)
		{
			vertexArr->push_back(subCenter + tmpSubVec);
			tmpSubVec = torusQuat * tmpSubVec;

			int size = vertexArr->size();
			normalArr->push_back(tangNormal ^ ((*vertexArr)[size - subCount - 1] - vertexArr->back()));
			normalArr->back().normalize();
			tangNormal = torusQuat * tangNormal;

			pDrawEle->push_back(size - subCount - 1);
			pDrawEle->push_back(size - 1);
		}
		pDrawEle->push_back(vertexArr->size() - subCount);
		// 最后一圈法向
		for (int i = 0; i < subCount; ++i)
			normalArr->push_back((*normalArr)[normalArr->size() - subCount]);
		geometry->addPrimitiveSet(pDrawEle);

		if (topVis)
		{
			size_t first = vertexArr->size();
			size_t base = first - subCount;
			osg::Vec3 topNormal = -faceNormal;
			vertexArr->push_back(subCenter);
			normalArr->push_back(topNormal);
			for (int i = 0; i < subCount; ++i)
			{
				vertexArr->push_back((*vertexArr)[base + i]);
				normalArr->push_back(topNormal);
			}
			vertexArr->push_back((*vertexArr)[vertexArr->size() - subCount]);
			normalArr->push_back(topNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		if (bottomVis)
		{
			size_t first = vertexArr->size();
			osg::Vec3 bottomNormal = torusNormal ^ normal;
			vertexArr->push_back(startPnt);
			normalArr->push_back(bottomNormal);
			for (int i = 0; i < subCount; ++i)
			{
				vertexArr->push_back((*vertexArr)[i]);
				normalArr->push_back(bottomNormal);
			}
			vertexArr->push_back((*vertexArr)[vertexArr->size() - subCount]);
			normalArr->push_back(bottomNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildRectangularTorus(const osg::Vec3 &center, const osg::Vec3 &startPnt, const osg::Vec3 &normal,
		double width, double height, double angle, const osg::Vec4 &color, bool topVis /*= true*/, bool bottomVis /*= true*/)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		bool isFull = equivalent(angle, 2 * M_PI, g_epsilon);
		if (isFull)
		{
			angle = 2 * M_PI;
		}

		Vec3 mainVec = startPnt - center;
		double mainRadius = mainVec.length() + width / 2.0;
		double mainIncAng = 2 * acos((mainRadius - g_deflection) / mainRadius);
		int mainCount = (int)ceil(angle / mainIncAng);
		mainIncAng = angle / mainCount;
		Quat mainQuat(mainIncAng, normal);

		Vec3 subVec = mainVec;
		subVec.normalize();
		Vec3 prevPntArr[4], postPntArr[4];
		prevPntArr[0] = startPnt - subVec * width / 2.0 - normal * height / 2.0;
		prevPntArr[1] = startPnt + subVec * width / 2.0 - normal * height / 2.0;
		prevPntArr[2] = startPnt + subVec * width / 2.0 + normal * height / 2.0;
		prevPntArr[3] = startPnt - subVec * width / 2.0 + normal * height / 2.0;

		Vec3 postOutNormal = subVec, postInNormal = -subVec;
		Vec3 topNormal;
		if (isFull)
		{
			for (int i = 0; i < 4; ++i)
				postPntArr[i] = prevPntArr[i];
		}
		else
		{
			Quat fullQuat(angle, normal);
			for (int i = 0; i < 4; ++i)
				postPntArr[i] = center + fullQuat * (prevPntArr[i] - center);
			postOutNormal = fullQuat * subVec;
			postInNormal = fullQuat * (-subVec);
			topNormal = normal ^ (fullQuat * subVec);
		}

		if (!isFull && bottomVis)
		{
			const GLint first = vertexArr->size();
			Vec3 bottomNormal = subVec ^ normal;
			for (int i = 0; i < 4; ++i)
			{
				vertexArr->push_back(prevPntArr[i]);
				normalArr->push_back(bottomNormal);
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));
		}

		// 底
		Vec3 vec1 = prevPntArr[0] - center;
		Vec3 vec2 = prevPntArr[1] - center;
		Vec3 subNormal = -normal;
		GLint first = vertexArr->size();
		for (int i = 0; i < mainCount; ++i)
		{
			vertexArr->push_back(center + vec1);
			normalArr->push_back(subNormal);
			vec1 = mainQuat * vec1;

			vertexArr->push_back(center + vec2);
			normalArr->push_back(subNormal);
			vec2 = mainQuat * vec2;
		}
		vertexArr->push_back(postPntArr[0]);
		normalArr->push_back(subNormal);

		vertexArr->push_back(postPntArr[1]);
		normalArr->push_back(subNormal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// 外侧
		vec1 = prevPntArr[1] - center;
		vec2 = prevPntArr[2] - center;
		subNormal = subVec;
		first = vertexArr->size();
		for (int i = 0; i < mainCount; ++i)
		{
			vertexArr->push_back(center + vec1);
			normalArr->push_back(subNormal);
			vec1 = mainQuat * vec1;

			vertexArr->push_back(center + vec2);
			normalArr->push_back(subNormal);
			vec2 = mainQuat * vec2;

			subNormal = mainQuat * subNormal;
		}
		vertexArr->push_back(postPntArr[1]);
		normalArr->push_back(postOutNormal);

		vertexArr->push_back(postPntArr[2]);
		normalArr->push_back(postOutNormal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// 顶
		vec1 = prevPntArr[2] - center;
		vec2 = prevPntArr[3] - center;
		subNormal = normal;
		first = vertexArr->size();
		for (int i = 0; i < mainCount; ++i)
		{
			vertexArr->push_back(center + vec1);
			normalArr->push_back(subNormal);
			vec1 = mainQuat * vec1;

			vertexArr->push_back(center + vec2);
			normalArr->push_back(subNormal);
			vec2 = mainQuat * vec2;
		}
		vertexArr->push_back(postPntArr[2]);
		normalArr->push_back(subNormal);

		postPntArr[3] = center + vec2;
		vertexArr->push_back(postPntArr[3]);
		normalArr->push_back(subNormal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// 内侧
		vec1 = prevPntArr[3] - center;
		vec2 = prevPntArr[0] - center;
		subNormal = -subVec;
		first = vertexArr->size();
		for (int i = 0; i < mainCount; ++i)
		{
			vertexArr->push_back(center + vec1);
			normalArr->push_back(subNormal);
			vec1 = mainQuat * vec1;

			vertexArr->push_back(center + vec2);
			normalArr->push_back(subNormal);
			vec2 = mainQuat * vec2;

			subNormal = mainQuat * subNormal;
		}
		vertexArr->push_back(postPntArr[3]);
		normalArr->push_back(postInNormal);

		vertexArr->push_back(postPntArr[0]);
		normalArr->push_back(postInNormal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		if (!isFull && topVis)
		{
			const GLint first = vertexArr->size();
			for (int i = 0; i < 4; ++i)
			{
				vertexArr->push_back(postPntArr[i]);
				normalArr->push_back(topNormal);
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildRectangularTorus(const osg::Vec3 &center, const osg::Vec3 &startPnt, const osg::Vec3 &normal,
		double startWidth, double startHeight, double endWidth, double endHeight, double angle, const osg::Vec4 &color,
		bool topVis /*= true*/, bool bottomVis /*= true*/)
	{
		if (equivalent(startWidth, endWidth, g_epsilon)
			&& equivalent(startHeight, endHeight, g_epsilon))
		{
			return BuildRectangularTorus(center, startPnt, normal, startWidth, startHeight, angle, color, topVis, bottomVis);
		}

		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		bool isFull = equivalent(angle, 2 * M_PI, g_epsilon);
		if (isFull)
		{
			angle = 2 * M_PI;
		}

		int mainCount = (int)ceil(angle / g_defaultIncAngle);
		double mainIncAngle = angle / mainCount;

		osg::Vec3 mainVec = startPnt - center;
		osg::Quat mainQuat(mainIncAngle, normal);

		osg::Vec3 torusNormal = mainVec;
		torusNormal.normalize();

		// 第一圈
		osg::DrawElementsUShort *pOutSideDrawEle = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0);
		osg::DrawElementsUShort *pTopDrawEle = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0);
		osg::DrawElementsUShort *pInSideDrawEle = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0);
		osg::DrawElementsUShort *pBottomDrawEle = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, 0);
		double width = startWidth, height = startHeight;
		osg::Vec3 subCenter = startPnt;
		osg::Vec3 halfWidthVec = torusNormal * width / 2.0;
		osg::Vec3 halfHeightVec = normal * height / 2.0;

		vertexArr->push_back(subCenter - halfWidthVec - halfHeightVec);
		pBottomDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(vertexArr->back());
		pInSideDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(subCenter + halfWidthVec - halfHeightVec);
		pBottomDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(vertexArr->back());
		pOutSideDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(subCenter + halfWidthVec + halfHeightVec);
		pOutSideDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(vertexArr->back());
		pTopDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(subCenter - halfWidthVec + halfHeightVec);
		pTopDrawEle->push_back(vertexArr->size() - 1);

		vertexArr->push_back(vertexArr->back());
		pInSideDrawEle->push_back(vertexArr->size() - 1);

		double widthFactor = (endWidth - startWidth) / mainCount;
		double heightFactor = (endHeight - startHeight) / mainCount;
		for (int i = 1; i < mainCount; ++i)
		{
			mainVec = mainQuat * mainVec;
			torusNormal = mainQuat * torusNormal;
			subCenter = center + mainVec;

			width += widthFactor;
			height += heightFactor;
			halfWidthVec = torusNormal * width / 2.0;
			halfHeightVec = normal * height / 2.0;

			vertexArr->push_back(subCenter - halfWidthVec - halfHeightVec);
			size_t size = vertexArr->size();
			pBottomDrawEle->push_back(size - 1);
			osg::Vec3 bottomNormal = ((*vertexArr)[size - 9] - vertexArr->back()) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 2]);
			bottomNormal.normalize();
			normalArr->push_back(bottomNormal);

			vertexArr->push_back(vertexArr->back());
			size = vertexArr->size();
			pInSideDrawEle->push_back(size - 1);
			osg::Vec3 inSideNormal = (vertexArr->back() - (*vertexArr)[size - 9]) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 6]);
			inSideNormal.normalize();
			normalArr->push_back(inSideNormal);

			vertexArr->push_back(subCenter + halfWidthVec - halfHeightVec);
			size = vertexArr->size();
			pBottomDrawEle->push_back(size - 1);
			normalArr->push_back(bottomNormal);

			vertexArr->push_back(vertexArr->back());
			size = vertexArr->size();
			pOutSideDrawEle->push_back(size - 1);
			osg::Vec3 outSideNormal = ((*vertexArr)[size - 9] - vertexArr->back()) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 1]);
			outSideNormal.normalize();
			normalArr->push_back(outSideNormal);

			vertexArr->push_back(subCenter + halfWidthVec + halfHeightVec);
			size = vertexArr->size();
			pOutSideDrawEle->push_back(size - 1);
			normalArr->push_back(outSideNormal);

			vertexArr->push_back(vertexArr->back());
			size = vertexArr->size();
			pTopDrawEle->push_back(size - 1);
			osg::Vec3 topNormal = ((*vertexArr)[size - 9] - vertexArr->back()) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 1]);
			topNormal.normalize();
			normalArr->push_back(topNormal);

			vertexArr->push_back(subCenter - halfWidthVec + halfHeightVec);
			size = vertexArr->size();
			pTopDrawEle->push_back(size - 1);
			normalArr->push_back(topNormal);

			vertexArr->push_back(vertexArr->back());
			size = vertexArr->size();
			pInSideDrawEle->push_back(size - 1);
			normalArr->push_back(inSideNormal);
		}

		// 最后一圈
		osg::Quat fullQuat(angle, normal);
		mainVec = fullQuat * (startPnt - center);
		torusNormal = mainVec;
		torusNormal.normalize();
		subCenter = center + mainVec;

		halfWidthVec = torusNormal * endWidth / 2.0;
		halfHeightVec = normal * endHeight / 2.0;

		vertexArr->push_back(subCenter - halfWidthVec - halfHeightVec);
		size_t size = vertexArr->size();
		pBottomDrawEle->push_back(size - 1);
		osg::Vec3 bottomNormal = ((*vertexArr)[size - 9] - vertexArr->back()) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 2]);
		bottomNormal.normalize();
		normalArr->push_back(bottomNormal);

		vertexArr->push_back(vertexArr->back());
		size = vertexArr->size();
		pInSideDrawEle->push_back(size - 1);
		osg::Vec3 inSideNormal = (vertexArr->back() - (*vertexArr)[size - 9]) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 6]);
		inSideNormal.normalize();
		normalArr->push_back(inSideNormal);

		vertexArr->push_back(subCenter + halfWidthVec - halfHeightVec);
		size = vertexArr->size();
		pBottomDrawEle->push_back(size - 1);
		normalArr->push_back(bottomNormal);

		vertexArr->push_back(vertexArr->back());
		size = vertexArr->size();
		pOutSideDrawEle->push_back(size - 1);
		osg::Vec3 outSideNormal = ((*vertexArr)[size - 9] - vertexArr->back()) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 1]);
		outSideNormal.normalize();
		normalArr->push_back(outSideNormal);

		vertexArr->push_back(subCenter + halfWidthVec + halfHeightVec);
		size = vertexArr->size();
		pOutSideDrawEle->push_back(size - 1);
		normalArr->push_back(outSideNormal);

		vertexArr->push_back(vertexArr->back());
		size = vertexArr->size();
		pTopDrawEle->push_back(size - 1);
		osg::Vec3 topNormal = ((*vertexArr)[size - 9] - vertexArr->back()) ^ ((*vertexArr)[size - 9] - (*vertexArr)[size - 9 + 1]);
		topNormal.normalize();
		normalArr->push_back(topNormal);

		vertexArr->push_back(subCenter - halfWidthVec + halfHeightVec);
		size = vertexArr->size();
		pTopDrawEle->push_back(size - 1);
		normalArr->push_back(topNormal);

		vertexArr->push_back(vertexArr->back());
		size = vertexArr->size();
		pInSideDrawEle->push_back(size - 1);
		normalArr->push_back(inSideNormal);

		// 最后一圈法向
		normalArr->push_back(bottomNormal);
		normalArr->push_back(inSideNormal);
		normalArr->push_back(bottomNormal);
		normalArr->push_back(outSideNormal);
		normalArr->push_back(outSideNormal);
		normalArr->push_back(topNormal);
		normalArr->push_back(topNormal);
		normalArr->push_back(inSideNormal);

		geometry->addPrimitiveSet(pOutSideDrawEle);
		geometry->addPrimitiveSet(pTopDrawEle);
		geometry->addPrimitiveSet(pInSideDrawEle);
		geometry->addPrimitiveSet(pBottomDrawEle);

		if (topVis)
		{
			size_t first = vertexArr->size();
			size_t base = first - 8;
			osg::Vec3 topNormal = normal ^ torusNormal;
			topNormal.normalize();
			for (int i = 0; i < 8; i += 2)
			{
				vertexArr->push_back((*vertexArr)[base + i]);
				normalArr->push_back(topNormal);
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));
		}

		if (bottomVis)
		{
			size_t first = vertexArr->size();
			osg::Vec3 bottomNormal = (startPnt - center) ^ normal;
			bottomNormal.normalize();
			for (int i = 0; i < 8; i += 2)
			{
				vertexArr->push_back((*vertexArr)[i]);
				normalArr->push_back(bottomNormal);
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildCone(const osg::Vec3 &center, const osg::Vec3 &height, double radius, const Vec4 &color, bool bottomVis /*= true*/)
	{
		osg::Vec3 bottomNormal = -height;
		bottomNormal.normalize();
		return BuildCone(center, bottomNormal, height, radius, color, bottomVis);
	}

	osg::ref_ptr<osg::Geometry> BuildCone(const osg::Vec3 &center, const osg::Vec3 &height, const osg::Vec3 &offset,
		double radius, const Vec4 &color, bool bottomVis /*= true*/)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		Vec3 bottomNormal = -height;
		bottomNormal.normalize();
		Vec3 xVec = bottomNormal ^ osg::Z_AXIS;
		if (xVec.length2() < g_epsilon)
			xVec = osg::X_AXIS;
		double incAng = 0.0;
		if (radius < g_deflection)
			incAng = 60.0 * M_PI / 180.0;
		else
			incAng = 2 * acos((radius - g_deflection) / radius);
		int count = (int)ceil(2 * M_PI / incAng);
		incAng = 2 * M_PI / count;
		Quat quat(incAng, bottomNormal);

		Vec3 yVec = height ^ xVec;
		yVec.normalize();
		vector<Vec3> pntArr;
		xVec *= radius;
		Vec3 topPnt = center + height;
		const GLint first = vertexArr->size();
		for (int i = 0; i < count; ++i)
		{
			Vec3 pnt = center + xVec;
			pntArr.push_back(pnt);

			vertexArr->push_back(topPnt);
			vertexArr->push_back(pnt);

			Vec3 normal = yVec ^ (topPnt - pnt);
			normal.normalize();
			normalArr->push_back(normal);
			normalArr->push_back(normal);

			xVec = quat * xVec;
			yVec = quat * yVec;
		}
		size_t pntCount = pntArr.size();

		vertexArr->push_back(topPnt);
		vertexArr->push_back(pntArr[0]);
		normalArr->push_back((*normalArr)[first]);
		normalArr->push_back((*normalArr)[first]);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		if (bottomVis)
		{
			const GLint first = vertexArr->size();
			vertexArr->push_back(center);
			normalArr->push_back(bottomNormal);
			for (size_t i = 0; i < pntCount; ++i)
			{
				vertexArr->push_back(pntArr[i]);
				normalArr->push_back(bottomNormal);
			}
			vertexArr->push_back(pntArr[0]);
			normalArr->push_back(bottomNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildSnout(const osg::Vec3 &center, const osg::Vec3 &height, const osg::Vec3 &offset,
		double bottomRadius, const double topRadius, const osg::Vec4 &color, bool bottomVis /*= true*/, bool topVis /*= true*/)
	{
		if (equivalent(bottomRadius, 0.0, g_epsilon))
		{
			return BuildCone(center + height + offset, -height, -offset, topRadius, color, topVis);
		}
		else if (equivalent(topRadius, 0.0, g_epsilon))
		{
			return BuildCone(center, height, offset, bottomRadius, color, bottomVis);
		}

		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		Vec3 bottomNormal = -height;
		bottomNormal.normalize();
		Vec3 xVec = bottomNormal ^ osg::Z_AXIS;
		if (xVec.length2() < g_epsilon)
			xVec = osg::X_AXIS;
		double mainRadius = 0.0;
		if (bottomRadius > topRadius)
			mainRadius = bottomRadius;
		else
			mainRadius = topRadius;
		double incAng = 0.0;
		if (mainRadius < g_deflection)
			incAng = 60.0 * M_PI / 180.0;
		else
			incAng = 2.0 * acos((mainRadius - g_deflection) / mainRadius);
		int count = (int)ceil(2 * M_PI / incAng);
		incAng = 2 * M_PI / count;
		Quat quat(incAng, bottomNormal);

		Vec3 topCenter = center + height + offset;
		vector<Vec3> bottomPntArr, topPntArr;
		for (int i = 0; i < count; ++i)
		{
			bottomPntArr.push_back(center + xVec * bottomRadius);
			topPntArr.push_back(topCenter + xVec * topRadius);

			xVec = quat * xVec;
		}
		bottomPntArr.push_back(bottomPntArr[0]);
		topPntArr.push_back(topPntArr[0]);
		size_t pntCount = bottomPntArr.size();

		if (bottomVis)
		{
			const GLint first = vertexArr->size();
			vertexArr->push_back(center);
			normalArr->push_back(bottomNormal);
			for (size_t i = 0; i < pntCount; ++i)
			{
				vertexArr->push_back(bottomPntArr[i]);
				normalArr->push_back(bottomNormal);
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		if (topVis)
		{
			const GLint first = vertexArr->size();
			Vec3 topNormal = -bottomNormal;
			vertexArr->push_back(topCenter);
			normalArr->push_back(topNormal);
			for (size_t i = 0; i < pntCount; ++i)
			{
				vertexArr->push_back(topPntArr[i]);
				normalArr->push_back(topNormal);
			}
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		Vec3 yVec = height ^ xVec;
		yVec.normalize();
		const GLint first = vertexArr->size();
		for (size_t i = 0; i < pntCount; ++i)
		{
			vertexArr->push_back(topPntArr[i]);
			vertexArr->push_back(bottomPntArr[i]);

			Vec3 normal = yVec ^ (topPntArr[i] - bottomPntArr[i]);
			normal.normalize();
			normalArr->push_back(normal);
			normalArr->push_back(normal);

			yVec = quat * yVec;
		}
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildPyramid(const osg::Vec3 &org, const osg::Vec3 &height, const osg::Vec3 &xAxis, const osg::Vec3 &offset,
		double bottomXLen, double bottomYLen, double topXLen, double topYLen, const osg::Vec4 &color)
	{
		osg::Vec3 yAxis = height ^ xAxis;
		yAxis.normalize();
		osg::Vec3 zAxis = height / height.length();
		osg::Vec3 topOrg = org + height + offset;

		osg::Vec3 p1 = org - xAxis * bottomXLen / 2.0 - yAxis * bottomYLen / 2.0;
		osg::Vec3 p2 = org + xAxis * bottomXLen / 2.0 - yAxis * bottomYLen / 2.0;
		osg::Vec3 p3 = org + xAxis * bottomXLen / 2.0 + yAxis * bottomYLen / 2.0;
		osg::Vec3 p4 = org - xAxis * bottomXLen / 2.0 + yAxis * bottomYLen / 2.0;

		osg::Vec3 p5 = topOrg - xAxis * topXLen / 2.0 - yAxis * topYLen / 2.0;
		osg::Vec3 p6 = topOrg + xAxis * topXLen / 2.0 - yAxis * topYLen / 2.0;
		osg::Vec3 p7 = topOrg + xAxis * topXLen / 2.0 + yAxis * topYLen / 2.0;
		osg::Vec3 p8 = topOrg - xAxis * topXLen / 2.0 + yAxis * topYLen / 2.0;

		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		vertexArr->push_back(p1);
		vertexArr->push_back(p2);
		vertexArr->push_back(p3);
		vertexArr->push_back(p4);
		normalArr->push_back(-zAxis);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normalArr->back());

		vertexArr->push_back(p5);
		vertexArr->push_back(p6);
		vertexArr->push_back(p7);
		vertexArr->push_back(p8);
		normalArr->push_back(zAxis);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normalArr->back());

		vertexArr->push_back(p1);
		vertexArr->push_back(p2);
		vertexArr->push_back(p6);
		vertexArr->push_back(p5);
		normalArr->push_back((p6 - p1) ^ (p5 - p2));
		normalArr->back().normalize();
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normalArr->back());

		vertexArr->push_back(p2);
		vertexArr->push_back(p3);
		vertexArr->push_back(p7);
		vertexArr->push_back(p6);
		normalArr->push_back((p7 - p2) ^ (p6 - p3));
		normalArr->back().normalize();
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normalArr->back());

		vertexArr->push_back(p3);
		vertexArr->push_back(p4);
		vertexArr->push_back(p8);
		vertexArr->push_back(p7);
		normalArr->push_back((p8 - p3) ^ (p7 - p4));
		normalArr->back().normalize();
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normalArr->back());

		vertexArr->push_back(p4);
		vertexArr->push_back(p1);
		vertexArr->push_back(p5);
		vertexArr->push_back(p8);
		normalArr->push_back((p5 - p4) ^ (p8 - p1));
		normalArr->back().normalize();
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normalArr->back());

		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, vertexArr->size()));

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildSphere(const osg::Vec3 &center, const osg::Vec3 &height,
		double bottomRadius, const osg::Vec4 &color, bool bottomVis /*= true*/)
	{
		double h = height.length();
		double sphereRadius = (bottomRadius * bottomRadius + h * h) / 2.0 / h;
		double angle = M_PI / 2.0 - asin(2.0 * bottomRadius * h / (bottomRadius * bottomRadius + h * h));
		if (bottomRadius >= h)
		{
			angle = M_PI - angle * 2.0;
		}
		else
		{
			angle = M_PI + angle * 2.0;
		}
		Vec3 bottomNormal = -height;
		bottomNormal.normalize();
		return BuildSphere(center, bottomNormal, sphereRadius, angle, color, bottomVis);
	}

	osg::ref_ptr<osg::Geometry> BuildSphere(const osg::Vec3 &center, const osg::Vec3 &bottomNormal,
		double sphereRadius, double angle, const osg::Vec4 &color, bool bottomVis /*= true*/)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		bool isFull = equivalent(angle, 2 * M_PI, g_epsilon);
		if (isFull)
		{
			angle = 2 * M_PI;
		}

		Quat quat;
		quat.makeRotate(osg::Z_AXIS, bottomNormal);
		Vec3 xVec = quat * osg::X_AXIS;
		Vec3 yVec = xVec ^ bottomNormal;
		double incAng = 2 * acos((sphereRadius - g_deflection) / sphereRadius);
		int hCount = (int)ceil(2 * M_PI / incAng);
		double hIncAng = 2 * M_PI / hCount;
		Quat hQuat(hIncAng, -bottomNormal);

		int vCount = (int)ceil(angle / incAng);
		if (vCount & 1) // 如果是奇数，则变成偶数
			++vCount;
		double vIncAng = angle / vCount;
		Quat vQuat(vIncAng, yVec);

		quat.makeRotate(-angle / 2.0, yVec);
		Vec3 vec1 = quat * (-bottomNormal) * sphereRadius;
		Vec3 vec2 = vQuat * vec1;
		if (!isFull && bottomVis)
		{
			const GLint first = vertexArr->size();
			Vec3 bVec = vec1;
			Vec3 bottomCenter;
			if (angle > M_PI)
			{
				double len = sphereRadius * sin((angle - M_PI) / 2.0);
				bottomCenter = center + bottomNormal * len;
			}
			else
			{
				double len = sphereRadius * sin((M_PI - angle) / 2.0);
				bottomCenter = center - bottomNormal * len;
			}
			vertexArr->push_back(bottomCenter);
			normalArr->push_back(bottomNormal);
			for (int i = 0; i < hCount; ++i)
			{
				vertexArr->push_back(center + bVec);
				normalArr->push_back(bottomNormal);
				bVec = hQuat * bVec;
			}
			vertexArr->push_back((*vertexArr)[first + 1]);
			normalArr->push_back(bottomNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		const GLint first = vertexArr->size();
		for (int i = 0; i < vCount / 2; ++i)
		{
			Vec3 hVec1 = vec1;
			Vec3 hVec2 = vec2;
			const size_t hFirst = vertexArr->size();
			for (int j = 0; j < hCount; ++j)
			{
				vertexArr->push_back(center + hVec1);
				vertexArr->push_back(center + hVec2);
				normalArr->push_back(hVec1);
				normalArr->back().normalize();
				normalArr->push_back(hVec2);
				normalArr->back().normalize();

				hVec1 = hQuat * hVec1;
				hVec2 = hQuat * hVec2;
			}
			vertexArr->push_back((*vertexArr)[hFirst]);
			vertexArr->push_back((*vertexArr)[hFirst + 1]);
			normalArr->push_back((*normalArr)[hFirst]);
			normalArr->push_back((*normalArr)[hFirst + 1]);

			vec1 = vec2;
			vec2 = vQuat * vec2;
		}
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildEllipsoid(const osg::Vec3 &center, const osg::Vec3 &aLen,
		double bRadius, double angle, const osg::Vec4 &color, bool bottomVis /*= true*/)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		bool isFull = equivalent(angle, 2 * M_PI, g_epsilon);
		if (isFull)
		{
			angle = 2 * M_PI;
		}

		Vec3 bottomNormal = -aLen;
		bottomNormal.normalize();
		Quat localToWold;
		localToWold.makeRotate(osg::Z_AXIS, -bottomNormal);
		Vec3 xVec = localToWold * osg::X_AXIS;
		Vec3 yVec = xVec ^ bottomNormal;
		double incAng = 2 * acos((bRadius - g_deflection) / bRadius);
		int hCount = (int)ceil(2 * M_PI / incAng);
		double hIncAng = 2 * M_PI / hCount;
		Quat hQuat(hIncAng, -bottomNormal);

		int vCount = (int)ceil(angle / incAng);
		if (vCount & 1) // 如果是奇数，则变成偶数
			++vCount;
		double vIncAng = angle / vCount;

		double currAngle = angle / 2.0;
		double b = bRadius;
		double a = aLen.length();
		Vec3 vec1(b * sin(currAngle), 0, a * cos(currAngle));
		vec1 = localToWold * vec1;
		Vec3 normal1(sin(currAngle) / b, 0, cos(currAngle) / a);
		normal1 = localToWold * normal1;
		normal1.normalize();

		currAngle -= vIncAng;
		Vec3 vec2(b * sin(currAngle), 0, a * cos(currAngle));
		vec2 = localToWold * vec2;
		Vec3 normal2(sin(currAngle) / b, 0, cos(currAngle) / a);
		normal2 = localToWold * normal2;
		normal2.normalize();

		const GLint first = vertexArr->size();
		for (int i = 0; i < vCount / 2; ++i)
		{
			Vec3 hVec1 = vec1;
			Vec3 hVec2 = vec2;
			Vec3 hNormal1 = normal1;
			Vec3 hNormal2 = normal2;
			const size_t hFirst = vertexArr->size();
			for (int j = 0; j < hCount; ++j)
			{
				vertexArr->push_back(center + hVec1);
				vertexArr->push_back(center + hVec2);
				normalArr->push_back(hNormal1);
				normalArr->push_back(hNormal2);

				hVec1 = hQuat * hVec1;
				hVec2 = hQuat * hVec2;

				hNormal1 = hQuat * hNormal1;
				hNormal2 = hQuat * hNormal2;
			}
			vertexArr->push_back((*vertexArr)[hFirst]);
			vertexArr->push_back((*vertexArr)[hFirst + 1]);
			normalArr->push_back((*normalArr)[hFirst]);
			normalArr->push_back((*normalArr)[hFirst + 1]);

			vec1 = vec2;
			currAngle -= vIncAng;
			vec2.set(b * sin(currAngle), 0, a * cos(currAngle));
			vec2 = localToWold * vec2;

			normal1 = normal2;
			normal2.set(sin(currAngle) / b, 0, cos(currAngle) / a);
			normal2 = localToWold * normal2;
			normal2.normalize();
		}
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		if (!isFull && bottomVis)
		{
			const GLint first = vertexArr->size();
			currAngle = angle / 2.0;
			Vec3 vec1(b * sin(currAngle), 0, a * cos(currAngle));
			vec1 = localToWold * vec1;
			Vec3 vec2(0, 0, a * cos(currAngle));
			vec2 = localToWold * vec2;

			vertexArr->push_back(center + vec2);
			normalArr->push_back(bottomNormal);
			for (int i = 0; i < hCount; ++i)
			{
				vertexArr->push_back(center + vec1);
				normalArr->push_back(bottomNormal);
				vec1 = hQuat * vec1;
			}
			vertexArr->push_back((*vertexArr)[first + 1]);
			normalArr->push_back(bottomNormal);
			geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildWedge(const osg::Vec3 &org, const osg::Vec3 &edge1, const osg::Vec3 &edge2,
		const osg::Vec3 &height, const osg::Vec4 &color)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		// bottom
		osg::Vec3 bottomNormal = edge2 ^ edge1;
		bottomNormal.normalize();
		GLint first = vertexArr->size();
		vertexArr->push_back(org);
		vertexArr->push_back(org + edge1);
		vertexArr->push_back(org + edge2);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(bottomNormal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLES, first, vertexArr->size() - first));

		// top
		osg::Vec3 topNormal = -bottomNormal;
		first = vertexArr->size();
		vertexArr->push_back(org + height);
		vertexArr->push_back(org + edge1 + height);
		vertexArr->push_back(org + edge2 + height);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(topNormal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLES, first, vertexArr->size() - first));

		// face1
		osg::Vec3 normal = edge1 ^ height;
		normal.normalize();
		first = vertexArr->size();
		vertexArr->push_back((*vertexArr)[0]);
		vertexArr->push_back((*vertexArr)[1]);
		vertexArr->push_back((*vertexArr)[4]);
		vertexArr->push_back((*vertexArr)[3]);
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));

		// face2
		normal = height ^ edge2;
		normal.normalize();
		first = vertexArr->size();
		vertexArr->push_back((*vertexArr)[2]);
		vertexArr->push_back((*vertexArr)[0]);
		vertexArr->push_back((*vertexArr)[3]);
		vertexArr->push_back((*vertexArr)[5]);
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));

		// face3
		normal = (edge2 - edge1) ^ height;
		normal.normalize();
		first = vertexArr->size();
		vertexArr->push_back((*vertexArr)[1]);
		vertexArr->push_back((*vertexArr)[2]);
		vertexArr->push_back((*vertexArr)[5]);
		vertexArr->push_back((*vertexArr)[4]);
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildPrism(const osg::Vec3 &org, const osg::Vec3 &height,
		const osg::Vec3 &bottomStartPnt, int edgeNum, const osg::Vec4 &color)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		double incAng = 2.0 * M_PI / edgeNum;

		// bottom
		GLint first = vertexArr->size();
		osg::Vec3 vec = bottomStartPnt - org;
		osg::Vec3 normal = -height;
		normal.normalize();
		Quat quat(incAng, normal);
		vertexArr->push_back(org);
		for (int i = 0; i < edgeNum; ++i)
		{
			vertexArr->push_back(org + vec);
			vec = quat * vec;
		}
		vertexArr->push_back((*vertexArr)[first + 1]);
		for (int i = 0; i < edgeNum + 2; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));

		// top
		first = vertexArr->size();
		normal = -normal;
		for (int i = 0; i < edgeNum + 2; ++i)
		{
			vertexArr->push_back((*vertexArr)[i] + height);
			normalArr->push_back(normal);
		}
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));

		first = vertexArr->size();
		for (int i = 0; i < edgeNum; ++i)
		{
			vertexArr->push_back((*vertexArr)[i + 1]);
			vertexArr->push_back((*vertexArr)[i + 2]);
			vertexArr->push_back((*vertexArr)[i + edgeNum + 4]);
			vertexArr->push_back((*vertexArr)[i + edgeNum + 3]);

			normal = height ^ ((*vertexArr)[i + 2] - (*vertexArr)[i + 1]);
			normal.normalize();
			for (int j = 0; j < 4; ++j)
				normalArr->push_back(normal);
		}
		geometry->addPrimitiveSet(new DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildSCylinder(const osg::Vec3 &org, const osg::Vec3 &height, const osg::Vec3 &bottomNormal,
		double radius, const osg::Vec4 &color, bool bottomVis /*= true*/, bool topVis /*= true*/)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		int count = (int)ceil(2 * M_PI / g_defaultIncAngle);
		osg::Vec3 topNormal = height;
		topNormal.normalize();
		double angleCos = bottomNormal * (-topNormal) / bottomNormal.length() / topNormal.length();
		double angle = acos(angleCos);
		double a = radius;
		double b = radius / sin(M_PI_2 - angle);
		osg::Vec3 vec = bottomNormal ^ topNormal;
		vec.normalize();
		osg::Quat bottomQuat(g_defaultIncAngle, bottomNormal);
		double currAngle = 0;
		osg::Quat localToWorldQuat;
		localToWorldQuat.makeRotate(osg::Z_AXIS, bottomNormal);
		osg::Vec3 yAxis = localToWorldQuat * osg::Y_AXIS;
		osg::Quat localToWorldQuat2;
		localToWorldQuat2.makeRotate(yAxis, vec);
		localToWorldQuat *= localToWorldQuat2;
		osg::Vec3 bottomVec(b * sin(currAngle), a * cos(currAngle), 0);
		bottomVec = localToWorldQuat * bottomVec;
		//bottomVec = localToWorldQuat2 * bottomVec;

		osg::Quat topQuat(g_defaultIncAngle, topNormal);
		osg::Vec3 topVec = vec * radius;
		osg::Vec3 topCenter = org + height;
		size_t first = vertexArr->size();
		for (int i = 0; i < count; ++i)
		{
			vertexArr->push_back(topCenter + topVec);
			vertexArr->push_back(org + bottomVec);
			normalArr->push_back(topVec);
			normalArr->back().normalize();
			normalArr->push_back(normalArr->back());

			currAngle += g_defaultIncAngle;
			bottomVec.set(b * sin(currAngle), a * cos(currAngle), 0);
			bottomVec = localToWorldQuat * bottomVec;
			//bottomVec = localToWorldQuat2 * bottomVec;
			topVec = topQuat * topVec;
		}
		vertexArr->push_back((*vertexArr)[first]);
		vertexArr->push_back((*vertexArr)[first+1]);
		normalArr->push_back((*normalArr)[first]);
		normalArr->push_back((*normalArr)[first+1]);
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, first, vertexArr->size() - first));

		if (bottomVis)
		{
			first = vertexArr->size();
			vertexArr->push_back(org);
			normalArr->push_back(bottomNormal);
			for (int i = 0; i < count + 1; ++i)
			{
				vertexArr->push_back((*vertexArr)[i * 2 + 1]);
				normalArr->push_back(bottomNormal);
			}
			geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		if (topVis)
		{
			first = vertexArr->size();
			vertexArr->push_back(topCenter);
			normalArr->push_back(topNormal);
			for (int i = 0; i < count + 1; ++i)
			{
				vertexArr->push_back((*vertexArr)[i * 2]);
				normalArr->push_back(topNormal);
			}
			geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, first, vertexArr->size() - first));
		}

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildSaddle(const osg::Vec3 &org, const osg::Vec3 &xLen, double yLen, const osg::Vec3 &zLen,
		double radius, const osg::Vec4 &color)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		int count = (int)ceil(2 * M_PI / g_defaultIncAngle);

		osg::Vec3 yNormal = zLen ^ xLen;
		yNormal.normalize();
		osg::Vec3 yVec = yNormal * yLen;
		osg::Vec3 bp1 = org - xLen / 2.0 - yVec / 2.0;
		osg::Vec3 bp2 = org + xLen / 2.0 - yVec / 2.0;
		osg::Vec3 bp3 = org + xLen / 2.0 + yVec / 2.0;
		osg::Vec3 bp4 = org - xLen / 2.0 + yVec / 2.0;

		osg::Vec3 tp1 = bp1 + zLen, tp2 = bp2 + zLen, tp3 = bp3 + zLen, tp4 = bp4 + zLen;

		// bottom
		size_t first = vertexArr->size();
		vertexArr->push_back(bp1);
		vertexArr->push_back(bp2);
		vertexArr->push_back(bp3);
		vertexArr->push_back(bp4);
		osg::Vec3 normal = -zLen;
		normal.normalize();
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);

		// top
		double yLen_2 = yLen / 2.0;
		osg::Vec3 width = yNormal * (yLen_2 - radius);
		bool isCircLessThenRect = radius < yLen_2;
		if (isCircLessThenRect)
		{
			// 1
			vertexArr->push_back(tp1);
			vertexArr->push_back(tp2);
			vertexArr->push_back(tp2 + width);
			vertexArr->push_back(tp1 + width);

			// 2
			vertexArr->push_back(tp4 - width);
			vertexArr->push_back(tp3 - width);
			vertexArr->push_back(tp3);
			vertexArr->push_back(tp4);

			normal = -normal;
			for (int i = 0; i < 8; ++i)
				normalArr->push_back(normal);
		}

		// front
		vertexArr->push_back(tp4);
		vertexArr->push_back(tp3);
		vertexArr->push_back(bp3);
		vertexArr->push_back(bp4);
		normal = yNormal;
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);

		// back
		vertexArr->push_back(bp1);
		vertexArr->push_back(bp2);
		vertexArr->push_back(tp2);
		vertexArr->push_back(tp1);
		normal = -normal;
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);

		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));

		// calc top circ
		double angle = M_PI;
		osg::Vec3 circCenter;
		if (radius > yLen_2)
		{
			angle = asin(yLen_2 / radius);
			osg::Vec3 vec = zLen;
			vec.normalize();
			vec *= cos(angle) * radius;
			circCenter = tp1 + yVec / 2.0 + vec;
			angle *= 2.0;
		}
		else
		{
			circCenter = tp1 + yVec / 2.0;
		}
		count = (int)ceil(angle / g_defaultIncAngle);
		double incAng = angle / count;

		first = vertexArr->size();
		osg::Vec3 circPnt(tp1);
		if (isCircLessThenRect)
			circPnt = tp1 + width;
		osg::Vec3 circVec = circCenter - circPnt;
		osg::Quat quat(incAng, xLen);
		for (int i = 0; i < count + 1; ++i)
		{
			vertexArr->push_back(circCenter - circVec);
			vertexArr->push_back(vertexArr->back() + xLen);
			normalArr->push_back(circVec);
			normalArr->back().normalize();
			normalArr->push_back(normalArr->back());

			circVec = quat * circVec;
		}
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// left
		size_t leftFirst = vertexArr->size();
		double rectLen = yLen;
		osg::Vec3 rectPnt(bp1);
		if (isCircLessThenRect)
		{
			vertexArr->push_back(bp1);
			vertexArr->push_back(tp1);
			rectPnt = bp1 + width;
			rectLen = radius * 2.0;
		}
		osg::Vec3 rectVec = yNormal * rectLen / count;
		for (int i = 0; i < count + 1; ++i)
		{
			vertexArr->push_back(rectPnt);
			vertexArr->push_back((*vertexArr)[first + i * 2]);
			rectPnt += rectVec;
		}
		if (isCircLessThenRect)
		{
			vertexArr->push_back(bp4);
			vertexArr->push_back(tp4);
		}
		size_t leftEnd = vertexArr->size();
		normal = -xLen;
		normal.normalize();
		for (size_t i = leftFirst; i < leftEnd; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, leftFirst, vertexArr->size() - leftFirst));

		// right
		first = vertexArr->size();
		normal = -normal;
		for (size_t i = leftFirst; i < leftEnd; ++i)
		{
			vertexArr->push_back((*vertexArr)[i] + xLen);
			normalArr->push_back(normal);
		}

		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildRectCirc(const osg::Vec3 &rectCenter, const osg::Vec3 &xLen, double yLen, const osg::Vec3 &height,
		const osg::Vec3 &offset, double radius, const osg::Vec4 &color)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		osg::Vec3 topNormal = height;
		topNormal.normalize();
		osg::Vec3 yVec = height ^ xLen;
		yVec.normalize();
		yVec *= yLen;
		osg::Vec3 circVec = -yVec;
		circVec.normalize();
		circVec *= radius;
		osg::Vec3 circCenter = rectCenter + height + offset;

		int count = (int)ceil(M_PI * 2.0 / g_defaultIncAngle);
		int t = count % 4;
		if (t != 0)
			count += 4 - t;
		double incAng = M_PI * 2.0 / count;
		osg::Quat quat(incAng, topNormal);

		// top
		size_t topFirst = vertexArr->size();
		vertexArr->push_back(circCenter);
		normalArr->push_back(topNormal);
		for (int i = 0; i < count; ++i)
		{
			vertexArr->push_back(circCenter + circVec);
			normalArr->push_back(topNormal);
			circVec = quat * circVec;
		}
		vertexArr->push_back((*vertexArr)[topFirst + 1]);
		normalArr->push_back(topNormal);
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, topFirst, vertexArr->size() - topFirst));

		// bottom
		size_t first = vertexArr->size();
		osg::Vec3 bp1 = rectCenter - xLen / 2.0 - yVec / 2.0;
		osg::Vec3 bp2 = rectCenter + xLen / 2.0 - yVec / 2.0;
		osg::Vec3 bp3 = rectCenter + xLen / 2.0 + yVec / 2.0;
		osg::Vec3 bp4 = rectCenter - xLen / 2.0 + yVec / 2.0;
		vertexArr->push_back(bp1);
		vertexArr->push_back(bp2);
		vertexArr->push_back(bp3);
		vertexArr->push_back(bp4);
		osg::Vec3 normal = -topNormal;
		for (int i = 0; i < 4; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, first, vertexArr->size() - first));

		int count_4 = count >> 2;
		first = vertexArr->size();
		// front
		normal = yVec;
		normal.normalize();
		vertexArr->push_back(bp4);
		vertexArr->push_back(bp3);
		vertexArr->push_back((*vertexArr)[count_4 * 2 + topFirst + 1]);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normal);

		// back
		normal = -normal;
		vertexArr->push_back(bp1);
		vertexArr->push_back(bp2);
		vertexArr->push_back((*vertexArr)[topFirst + 1]);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normal);

		// right
		normal = xLen;
		normal.normalize();
		vertexArr->push_back(bp2);
		vertexArr->push_back(bp3);
		vertexArr->push_back((*vertexArr)[count_4 + topFirst + 1]);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normal);

		// left
		normal = -normal;
		vertexArr->push_back(bp4);
		vertexArr->push_back(bp1);
		vertexArr->push_back((*vertexArr)[count_4 * 3 + topFirst + 1]);
		for (int i = 0; i < 3; ++i)
			normalArr->push_back(normal);
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, first, vertexArr->size() - first));

		// back right
		first = vertexArr->size();
		size_t vertexIdx = topFirst + 1;
		osg::Vec3 vec = xLen;
		vec.normalize();
		for (int i = 0; i < count_4 + 1; ++i, ++vertexIdx)
		{
			vertexArr->push_back((*vertexArr)[vertexIdx]);
			vertexArr->push_back(bp2);
			normalArr->push_back((bp2 - (*vertexArr)[vertexIdx]) ^ vec);
			normalArr->back().normalize();
			normalArr->push_back(normalArr->back());

			vec = quat * vec;
		}
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// front right
		first = vertexArr->size();
		--vertexIdx;
		vec = yVec;
		vec.normalize();
		for (int i = 0; i < count_4 + 1; ++i, ++vertexIdx)
		{
			vertexArr->push_back((*vertexArr)[vertexIdx]);
			vertexArr->push_back(bp3);
			normalArr->push_back((bp3 - (*vertexArr)[vertexIdx]) ^ vec);
			normalArr->back().normalize();
			normalArr->push_back(normalArr->back());

			vec = quat * vec;
		}
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// front left
		first = vertexArr->size();
		--vertexIdx;
		vec = -xLen;
		vec.normalize();
		for (int i = 0; i < count_4 + 1; ++i, ++vertexIdx)
		{
			vertexArr->push_back((*vertexArr)[vertexIdx]);
			vertexArr->push_back(bp4);
			normalArr->push_back((bp4 - (*vertexArr)[vertexIdx]) ^ vec);
			normalArr->back().normalize();
			normalArr->push_back(normalArr->back());

			vec = quat * vec;
		}
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));

		// back left
		first = vertexArr->size();
		--vertexIdx;
		vec = -yVec;
		vec.normalize();
		for (int i = 0; i < count_4 + 1; ++i, ++vertexIdx)
		{
			vertexArr->push_back((*vertexArr)[vertexIdx]);
			vertexArr->push_back(bp1);
			normalArr->push_back((bp1 - (*vertexArr)[vertexIdx]) ^ vec);
			normalArr->back().normalize();
			normalArr->push_back(normalArr->back());

			vec = quat * vec;
		}
		geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, first, vertexArr->size() - first));
	
		return geometry;
	}

	osg::ref_ptr<osg::Geometry> BuildCombineGeometry(const CombineGeometry &cg)
	{
		ref_ptr<osg::Geometry> geometry = new osg::Geometry();
		ref_ptr<Vec3Array> vertexArr = new Vec3Array;
		ref_ptr<Vec3Array> normalArr = new Vec3Array;
		geometry->setVertexArray(vertexArr);
		geometry->setNormalArray(normalArr, osg::Array::BIND_PER_VERTEX);
		osg::ref_ptr<osg::Vec4Array> colArr = new osg::Vec4Array();
		colArr->push_back(cg.color);
		geometry->setColorArray(colArr, osg::Array::BIND_OVERALL);

		// shell
		std::vector<osg::Vec3> triVertexArr, triNormalArr;
		for each(const auto &shell in cg.shells)
		{
			for (size_t i = 0; i < shell->faces.size();)
			{
				if (shell->faces[i] == 4)
				{
					for (int j = 0; j < shell->faces[i]; ++j)
					{
						vertexArr->push_back(shell->vertexs[shell->faces[i + j + 1]]);
					}

					osg::Vec3 vec1 = shell->vertexs[shell->faces[i + 2]] - shell->vertexs[shell->faces[i + 1]];
					osg::Vec3 vec2 = shell->vertexs[shell->faces[i + 3]] - shell->vertexs[shell->faces[i + 2]];
					osg::Vec3 normal = vec1 ^ vec2;
					normal.normalize();
					for (int j = 0; j < shell->faces[i]; ++j)
						normalArr->push_back(normal);
				}
				else if (shell->faces[i] == 3)
				{
					for (int j = 0; j < shell->faces[i]; ++j)
					{
						triVertexArr.push_back(shell->vertexs[shell->faces[i + j + 1]]);
					}

					osg::Vec3 vec1 = shell->vertexs[shell->faces[i + 2]] - shell->vertexs[shell->faces[i + 1]];
					osg::Vec3 vec2 = shell->vertexs[shell->faces[i + 3]] - shell->vertexs[shell->faces[i + 2]];
					osg::Vec3 normal = vec1 ^ vec2;
					normal.normalize();
					for (int j = 0; j < shell->faces[i]; ++j)
						triNormalArr.push_back(normal);
				}

				i += shell->faces[i] + 1;
			}
		}

		// mesh
		for each (const auto &mesh in cg.meshs)
		{
			for (int i = 0; i < mesh->rows-1; ++i)
			{
				for (int j = 0; j < mesh->colums-1; ++j)
				{
					osg::Vec3 pnt1 = mesh->vertexs[i * mesh->colums + j];
					osg::Vec3 pnt2 = mesh->vertexs[i * mesh->colums + j + 1];
					osg::Vec3 pnt3 = mesh->vertexs[(i + 1) * mesh->colums + j + 1];
					osg::Vec3 pnt4 = mesh->vertexs[(i + 1) * mesh->colums + j];
					vertexArr->push_back(pnt1);
					vertexArr->push_back(pnt2);
					vertexArr->push_back(pnt3);
					vertexArr->push_back(pnt4);

					osg::Vec3 normal = (pnt2 - pnt1) ^ (pnt1 - pnt4);
					normal.normalize();
					for (int k = 0; k < 4; ++k)
						normalArr->push_back(normal);
				}
			}
		}

		if (!vertexArr->empty())
			geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, vertexArr->size()));

		size_t first = vertexArr->size();
		for (size_t i = 0; i < triVertexArr.size(); ++i)
		{
			vertexArr->push_back(triVertexArr[i]);
			normalArr->push_back(triNormalArr[i]);
		}
		if (!triVertexArr.empty())
			geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, first, vertexArr->size() - first));

		// polygon
		for each (const auto &polygon in cg.polygons)
		{
			first = vertexArr->size();
			osg::Vec3 normal = (polygon->vertexs[1] - polygon->vertexs[0]) ^ (polygon->vertexs[2] - polygon->vertexs[1]);
			normal.normalize();
			for (size_t i = 0; i < polygon->vertexs.size(); ++i)
			{
				vertexArr->push_back(polygon->vertexs[i]);
				normalArr->push_back(normal);
			}
			geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, first, vertexArr->size() - first));
		}

		return geometry;
	}

}
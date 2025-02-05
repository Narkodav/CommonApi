#include "../../include/Physics/Hitboxes.h"

namespace Physics
{
	const std::vector<std::string> Hitbox::hitboxTypes = {
		"parallelogram",
		"cylinder",
		"sphere",
		"compound",
	};

	bool ParallelogramHitbox::intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const
	{
		if (const CompoundHitbox* otherHitbox = dynamic_cast<const CompoundHitbox*>(&other))
			return otherHitbox->intersects(*this, otherPosition, thisPosition);
		else if (const CylinderHitbox* otherHitbox = dynamic_cast<const CylinderHitbox*>(&other))
			return IntersectionFunctions::intersectsParallelogramCylinder(*this, thisPosition, *otherHitbox, otherPosition);
		else if (const ParallelogramHitbox* otherHitbox = dynamic_cast<const ParallelogramHitbox*>(&other))
			return IntersectionFunctions::intersectsParallelogram(*this, thisPosition, *otherHitbox, otherPosition);
		else if (const SphereHitbox* otherHitbox = dynamic_cast<const SphereHitbox*>(&other))
			return IntersectionFunctions::intersectsParallelogramSphere(*this, thisPosition, *otherHitbox, otherPosition);
	}

	Physics::RayCasting::IntersectResult ParallelogramHitbox::intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const
	{
		return Physics::RayCasting::intersectsAxisAlignedBox(ray, { hitboxPosition, m_width, m_height, m_depth });
	}

	bool ParallelogramHitbox::contains(const glm::vec3& point, glm::vec3 thisPosition) const
	{
		glm::vec3 delta = thisPosition - point;
		return m_halfWidth > abs(delta.x) &&
			m_halfHeight > abs(delta.y) &&
			m_halfDepth > abs(delta.z);
	}

	bool CylinderHitbox::intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const
	{
		if (const CompoundHitbox* otherHitbox = dynamic_cast<const CompoundHitbox*>(&other))
			return otherHitbox->intersects(*this, otherPosition, thisPosition);
		else if (const CylinderHitbox* otherHitbox = dynamic_cast<const CylinderHitbox*>(&other))
			return IntersectionFunctions::intersectsCylinder(*this, thisPosition, *otherHitbox, otherPosition);
		else if (const ParallelogramHitbox* otherHitbox = dynamic_cast<const ParallelogramHitbox*>(&other))
			return IntersectionFunctions::intersectsParallelogramCylinder(*otherHitbox, otherPosition, *this, thisPosition);
		else if (const SphereHitbox* otherHitbox = dynamic_cast<const SphereHitbox*>(&other))
			return IntersectionFunctions::intersectsCylinderSphere(*this, thisPosition, *otherHitbox, otherPosition);
	}

	Physics::RayCasting::IntersectResult CylinderHitbox::intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const
	{
		return { 0 };
	}

	bool CylinderHitbox::contains(const glm::vec3& point, glm::vec3 thisPosition) const
	{
		glm::vec3 delta = thisPosition - point;
		return delta.x * delta.x + delta.z * delta.z < pow(m_radius + m_radius, 2) &&
			m_halfHeight > abs(delta.y);
	}

	bool SphereHitbox::intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const
	{
		if (const CompoundHitbox* otherHitbox = dynamic_cast<const CompoundHitbox*>(&other))
			return otherHitbox->intersects(*this, otherPosition, thisPosition);
		else if (const CylinderHitbox* otherHitbox = dynamic_cast<const CylinderHitbox*>(&other))
			return IntersectionFunctions::intersectsCylinderSphere(*otherHitbox, otherPosition, *this, thisPosition);
		else if (const ParallelogramHitbox* otherHitbox = dynamic_cast<const ParallelogramHitbox*>(&other))
			return IntersectionFunctions::intersectsParallelogramSphere(*otherHitbox, otherPosition, *this, thisPosition);
		else if (const SphereHitbox* otherHitbox = dynamic_cast<const SphereHitbox*>(&other))
			return IntersectionFunctions::intersectsSphere(*this, thisPosition, *otherHitbox, otherPosition);
	}

	Physics::RayCasting::IntersectResult SphereHitbox::intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const
	{
		return { 0 };
	}

	bool SphereHitbox::contains(const glm::vec3& point, glm::vec3 thisPosition) const
	{
		glm::vec3 local = point - thisPosition;
		return glm::dot(local, local) <= m_radius * m_radius;
	}

	inline bool IntersectionFunctions::intersectsParallelogramCylinder(
		const ParallelogramHitbox& para, glm::vec3 paraPosition,
		const CylinderHitbox& cyl, glm::vec3 cylPosition)
	{
		if (para.m_halfHeight + cyl.m_halfHeight <= abs(paraPosition.y - cylPosition.y))
			return 0;

		float dx = glm::clamp(cylPosition.x, paraPosition.x - para.m_halfWidth, paraPosition.x + para.m_halfWidth) - cylPosition.x;
		float dz = glm::clamp(cylPosition.z, paraPosition.z - para.m_halfDepth, paraPosition.z + para.m_halfDepth) - cylPosition.z;

		return dx * dx + dz * dz < cyl.m_radius * cyl.m_radius;
	}

	inline bool IntersectionFunctions::intersectsParallelogram(
		const ParallelogramHitbox& para1, glm::vec3 para1Position,
		const ParallelogramHitbox& para2, glm::vec3 para2Position)
	{
		glm::vec3 delta = para2Position - para1Position;
		return para1.m_halfWidth + para2.m_halfWidth > abs(delta.x) &&
			para1.m_halfHeight + para2.m_halfHeight > abs(delta.y) &&
			para1.m_halfDepth + para2.m_halfDepth > abs(delta.z);

	}

	inline bool IntersectionFunctions::intersectsCylinder(
		const CylinderHitbox& cyl1, glm::vec3 cyl1Position,
		const CylinderHitbox& cyl2, glm::vec3 cyl2Position)
	{
		glm::vec3 delta = cyl2Position - cyl1Position;
		return delta.x * delta.x + delta.z * delta.z < pow(cyl1.m_radius + cyl2.m_radius, 2) &&
			cyl1.m_halfHeight + cyl2.m_halfHeight > abs(delta.y);
	}

	inline bool IntersectionFunctions::intersectsSphere(
		const SphereHitbox& sph1, glm::vec3 sph1Position,
		const SphereHitbox& sph2, glm::vec3 sph2Position)
	{
		return glm::length(sph2Position - sph1Position) < sph1.m_radius + sph2.m_radius;
	}

	inline bool IntersectionFunctions::intersectsParallelogramSphere(
		const ParallelogramHitbox& para, glm::vec3 paraPosition,
		const SphereHitbox& sph, glm::vec3 sphPosition)
	{
		glm::vec3 sphereLocal = sphPosition - paraPosition;

		// Find closest point on parallelogram to sphere center
		glm::vec3 closest;

		// Clamp each component to the parallelogram's bounds
		closest.x = glm::clamp(sphereLocal.x, -para.m_halfWidth, para.m_halfWidth);
		closest.y = glm::clamp(sphereLocal.y, -para.m_halfHeight, para.m_halfHeight);
		closest.z = glm::clamp(sphereLocal.z, -para.m_halfDepth, para.m_halfDepth);

		// Calculate vector from closest point to sphere center
		glm::vec3 difference = sphereLocal - closest;

		// Check if squared distance is less than squared radius
		float distanceSquared = glm::dot(difference, difference);
		float radiusSquared = sph.m_radius * sph.m_radius;

		return distanceSquared <= radiusSquared;
	}

	inline bool IntersectionFunctions::intersectsCylinderSphere(
		const CylinderHitbox& cyl, glm::vec3 cylPosition,
		const SphereHitbox& sph, glm::vec3 sphPosition)
	{
		// Transform sphere position relative to cylinder
		glm::vec3 sphereLocal = sphPosition - cylPosition;

		// Check height (Y-axis) first
		float distanceY = std::abs(sphereLocal.y) - cyl.m_halfHeight;
		if (distanceY > sph.m_radius) {
			return 0; // Too far above or below cylinder
		}

		// Check radial distance (in XZ plane)
		// Remove Y component since we only care about distance from cylinder's central axis
		glm::vec2 radialDist = glm::vec2(sphereLocal.x, sphereLocal.z);
		float radialDistSq = glm::dot(radialDist, radialDist);
		float combinedRadius = cyl.m_radius + sph.m_radius;

		// If sphere center is above or below cylinder caps
		if (std::abs(sphereLocal.y) > cyl.m_halfHeight) {
			// Need to check distance to cap edge
			return radialDistSq <= combinedRadius * combinedRadius;
		}

		// Sphere center is at cylinder height, just check radial distance
		return radialDistSq <= combinedRadius * combinedRadius;


	}

	void CompoundHitbox::addHitbox(std::unique_ptr<Hitbox> hitbox, glm::vec3 position)
	{
		m_hitboxes.emplace_back(std::move(hitbox));
		m_positions.emplace_back(position);
	}

	bool CompoundHitbox::intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const
	{
		for (int i = 0; i < m_hitboxes.size(); i++)
			if (m_hitboxes[i]->intersects(other, thisPosition + m_positions[i], otherPosition))
				return 1;
		return 0;
	}

	Physics::RayCasting::IntersectResult CompoundHitbox::intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const
	{
		Physics::RayCasting::IntersectResult buffer;
		Physics::RayCasting::IntersectResult final = { 0 };
		float minDistance = INFINITY;

		for (int i = 0; i < m_hitboxes.size(); i++)
		{
			buffer = m_hitboxes[i]->intersectsRay(ray, hitboxPosition + m_positions[i]);
			if (buffer.intersects && buffer.distance < minDistance)
				final = buffer;
		}

		return final;
	}

	bool CompoundHitbox::contains(const glm::vec3& point, glm::vec3 thisPosition) const
	{
		for (int i = 0; i < m_hitboxes.size(); i++)
			if (m_hitboxes[i]->contains(point, thisPosition + m_positions[i]))
				return 1;
		return 0;
	}

	std::unique_ptr<Hitbox> HitboxFactory::createHitbox(const std::string& type, const std::vector<float>& params)
	{
		if (type == Hitbox::hitboxTypes[static_cast<unsigned int>(Hitbox::HitboxTypes::HITBOX_PARALLELOGRAM)])
			return std::make_unique<ParallelogramHitbox>(params[0], params[1], params[2]);
		else if (type == Hitbox::hitboxTypes[static_cast<unsigned int>(Hitbox::HitboxTypes::HITBOX_CYLINDER)])
			return std::make_unique<CylinderHitbox>(params[0], params[1]);
	}

	std::unique_ptr<Hitbox> HitboxFactory::createHitbox(const std::vector<std::string>& types, const std::vector<float>& params, const std::vector<glm::vec3>& positions)
	{
		std::unique_ptr<CompoundHitbox> hitbox = std::make_unique<CompoundHitbox>();
		unsigned int currentParamBord = 0;
		for (int i = 0; i < types.size(); i++)
		{
			if (types[i] == Hitbox::hitboxTypes[static_cast<unsigned int>(Hitbox::HitboxTypes::HITBOX_PARALLELOGRAM)])
			{
				hitbox->addHitbox(std::make_unique<ParallelogramHitbox>(params[currentParamBord], params[currentParamBord + 1], params[currentParamBord + 2]), positions[i]);
				currentParamBord += 3;
			}
			else if (types[i] == Hitbox::hitboxTypes[static_cast<unsigned int>(Hitbox::HitboxTypes::HITBOX_CYLINDER)])
			{
				hitbox->addHitbox(std::make_unique<CylinderHitbox>(params[currentParamBord], params[currentParamBord + 1]), positions[i]);
				currentParamBord += 2;
			}
		}
		return hitbox;
	}

	std::unique_ptr<Hitbox> HitboxFactory::loadHitbox(const std::string& filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			std::cerr << "Failed to open file: " << filepath << std::endl;
			return nullptr;
		}

		std::vector<float> params;
		std::string line;
		std::getline(file, line);
		if (line != Hitbox::hitboxTypes[static_cast<unsigned int>(Hitbox::HitboxTypes::HITBOX_COMPOUND)])
		{
			std::string type = line;
			unsigned int position = 0;
			std::getline(file, line);
			while (position < line.size())
				params.emplace_back(Utils::parseFloat(line, ':', position, &position));
			return createHitbox(type, params);
		}

		std::vector<std::string> types;
		std::vector<glm::vec3> positions;

		while (std::getline(file, line))
		{
			types.emplace_back(line);
			unsigned int position = 0;

			std::getline(file, line);
			while (position < line.size())
			{
				glm::vec3 buffer;
				buffer.x = Utils::parseFloat(line, ':', position, &position);
				buffer.y = Utils::parseFloat(line, ':', position, &position);
				buffer.z = Utils::parseFloat(line, ':', position, &position);
				positions.emplace_back(buffer);
			}

			position = 0;
			std::getline(file, line);
			while (position < line.size())
				params.emplace_back(Utils::parseFloat(line, ':', position, &position));
		}

		return createHitbox(types, params, positions);

	}
}
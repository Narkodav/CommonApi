#include "../../include/Utilities/Functions.h"

namespace Utilities
{
    float parseFloat(const std::string& str, char endValue /*= '\0'*/, unsigned int start /*= 0*/, unsigned int* stopValue /*= nullptr*/)
    {
        float result = 0.0f;
        float mult = 1;
        int power = 10;
        int i = start;
        if (str[start] == '-')
        {
            mult = -1;
            start++;
        }

        for (i; i < str.size(); i++)
        {
            if (str[i] == '.')
                break;
            else if (str[i] != endValue)
            {
                result *= 10;
                result += str[i] - '0';
            }
            else
            {
                *stopValue = i + 1;
                return result * mult;
            }
        }
        i++;
        for (i; i < str.size(); i++)
        {
            if (str[i] != endValue)
            {
                result += (float)(str[i] - '0') / (float)(power);
                power *= 10;
            }
            else
            {
                *stopValue = i + 1;
                return result * mult;
            }
        }

        *stopValue = i;
        return result * mult;
    }
    
    glm::ivec2 tileToChunkCoord(glm::ivec3 tileCoord, int chunkSize)
    {
        if (tileCoord.x >= 0)
            tileCoord.x = tileCoord.x / chunkSize;
        else tileCoord.x = (tileCoord.x + 1) / chunkSize - 1;

        if (tileCoord.z >= 0)
            tileCoord.y = tileCoord.z / chunkSize;
        else tileCoord.y = (tileCoord.z + 1) / chunkSize - 1;
        return tileCoord;
    }

    glm::ivec3 globalToLocal(glm::ivec3 tileCoord, int chunkSize)
    {
        if (tileCoord.x >= 0)
            tileCoord.x = tileCoord.x % chunkSize;
        else tileCoord.x = 15 - (-(tileCoord.x + 1) % chunkSize);

        if (tileCoord.z >= 0)
            tileCoord.z = tileCoord.z % chunkSize;
        else tileCoord.z = 15 - (-(tileCoord.z + 1) % chunkSize);
        return tileCoord;
    }

    std::vector<glm::ivec2> getSortedCircleCoords(unsigned int radius, float padding)
    {
        float radiusWithPadding = radius + padding;
        unsigned int loadDistanceSquaredWithCushion = pow(radiusWithPadding, 2);
        std::vector<glm::ivec2> loadArea;
        loadArea.emplace_back(glm::ivec2(0, 0));
        /*m_queue.push({ ChunkManagementQueue::QueueType::GENERATION, glm::ivec2(0, 0) });*/

        for (int i = 1; i <= radiusWithPadding; i++)
        {
            loadArea.emplace_back(glm::ivec2(i, 0));
            loadArea.emplace_back(glm::ivec2(-i, 0));
            loadArea.emplace_back(glm::ivec2(0, i));
            loadArea.emplace_back(glm::ivec2(0, -i));
        }

        int dx;

        for (int y = 1; y <= radiusWithPadding; y++)
        {
            dx = sqrt(loadDistanceSquaredWithCushion - y * y);

            for (int x = 1; x <= dx + 0.5; x++)
            {
                loadArea.emplace_back(glm::ivec2(x, y));
                loadArea.emplace_back(glm::ivec2(-x, y));
                loadArea.emplace_back(glm::ivec2(x, -y));
                loadArea.emplace_back(glm::ivec2(-x, -y));
            }
        }

        std::sort(loadArea.begin(), loadArea.end(),
            [](const glm::ivec2& a, const glm::ivec2& b) {
                glm::vec2 af = glm::vec2(a);
                glm::vec2 bf = glm::vec2(b);
                return glm::dot(af, af) < glm::dot(bf, bf);
            });
        return loadArea;
    }
};
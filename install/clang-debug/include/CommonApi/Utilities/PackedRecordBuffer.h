#include <cstddef>
#include <cstdint>
#include <concepts>
#include <array>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cstring>
#include <span>
#include <vector>

namespace Utilities {

    // An inline packed fixed size storage of metadata and corresponding trailing opaque variable sized data
    template<typename Metadata, typename T, size_t s_capacity> requires (std::is_trivial_v<T> && std::is_trivial_v<T>)
    class PackedRecordBuffer {
    public:
        struct Node {
            std::span<T> chunk;
            Metadata meta;
        };

    private:
        std::array<T, s_capacity> m_data;
        std::vector<Node> m_nodes;
        size_t m_occupied; // In bytes

    public:
        
        void pushBack(Metadata meta, std::span<const T> values) {
            Node node;
            node.meta = meta;
            node.chunk = std::span<T>(m_data.begin() + m_occupied, values.size());
            std::copy(values.begin(), values.end(), node.chunk.begin());
            m_nodes.push_back(node);
            m_occupied += values.size();
        }

        size_t size() const { return m_nodes.size(); }

        Node& operator[](size_t index) { return m_nodes[index]; }
        const Node& operator[](size_t index) const { return m_nodes[index]; }

        bool canFit(size_t addition) const {
            return addition <= s_capacity - m_occupied;
        }

        void clear() {
            m_nodes.clear();
            m_occupied = 0;
        }
    };
}
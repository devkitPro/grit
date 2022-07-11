/*
===============================================================================

 Copyright (C) 2022 gba-hpp contributors
 Copyright (C) 2022 Gustavo Valiente (gustavo.valiente@protonmail.com)
 zlib kicense.
 For conditions of distribution and use, see license-zlib.txt file.

===============================================================================
*/

#ifndef GBA_HUFF_H
#define GBA_HUFF_H

#include <array>
#include <vector>
#include <algorithm>

namespace gba_huff
{

struct bios_uncomp_header
{
    enum class type : std::uint32_t
    {
        lz77 = 0x10,
        huffman_4 = 0x24,
        huffman_8 = 0x28,
        rle = 0x30,
        filter_8 = 0x81,
        filter_16 = 0x82,
    };

    type type : 8;
    std::uint32_t length : 24;
};

struct result_type
{
    bios_uncomp_header header;
    std::vector<std::uint8_t> data;
};

namespace detail
{

template<typename T, std::size_t N, class Compare>
class priority_queue
{

public:
    void push(const T& value)
    {
        m_buffer[m_size++] = value;

        std::sort(m_buffer.begin(), std::next(m_buffer.begin(), m_size), Compare());
    }

    const T& top() const
    {
        return m_buffer[0];
    }

    std::size_t size() const
    {
        return m_size;
    }

    void pop()
    {
        if(! m_size)
        {
            return;
        }

        for(std::size_t ii = 1, il = m_size; ii < il; ++ii)
        {
            m_buffer[ii - 1] = m_buffer[ii];
        }

        --m_size;
    }

private:
    std::array<T, N> m_buffer{};
    std::size_t m_size{};
};

struct node_type
{
    std::uint8_t data{};
    std::size_t weight{};
    node_type* node0{};
    node_type* node1{};

    bool is_leaf() const
    {
        return node0 == nullptr && node1 == nullptr;
    }

    bool is_branch() const
    {
        return node0 != nullptr && node1 != nullptr;
    }
};

template<std::size_t N>
class node_allocator
{

public:
    node_type* alloc(std::uint8_t data, std::size_t weight)
    {
        node_type& node = m_buffer[m_size++];
        node.data = data;
        node.weight = weight;
        node.node0 = node.node1 = nullptr;
        return &node;
    }

    node_type* alloc(std::size_t weight, node_type* node0, node_type* node1)
    {
        node_type& node = m_buffer[m_size++];
        node.data = {};
        node.weight = weight;
        node.node0 = node0;
        node.node1 = node1;
        return &node;
    }

private:
    std::array<node_type, N> m_buffer{};
    std::size_t m_size{};
};

struct node_compare
{
    bool operator()(const node_type* node0, const node_type* node1) const
    {
        return node0->weight < node1->weight;
    }
};

struct bit_code
{
    bit_code operator+(int i) const
    {
        bit_code copy = {value, length};

        if(i)
        {
            copy.value |= (1ull << length);
        }

        ++copy.length;
        return copy;
    }

    std::uint64_t value{};
    std::size_t length{};
};

void encode(const node_type* node, bit_code code, std::array<bit_code, 0x100>& bit_codes)
{
    if(node->is_branch())
    {
        encode(node->node0, code + 0, bit_codes);
        encode(node->node1, code + 1, bit_codes);
    }
    else
    {
        bit_codes[static_cast<int>(node->data)] = code;
    }
}

template<std::size_t N>
class flat_node_tree
{

public:
    explicit flat_node_tree(const node_type* node)
    {
        m_buffer[m_size++] = node;
        flatten(node);
    }

    const node_type* operator[](std::size_t idx) const
    {
        return m_buffer[idx];
    }

    std::size_t size() const
    {
        return m_size;
    }

    std::size_t index_of(const node_type* node) const
    {
        for(std::size_t ii = 0, il = m_size; ii < il; ++ii)
        {
            if(m_buffer[ii] == node)
            {
                return ii;
            }
        }

        return std::size_t(-1);
    }

    std::size_t index_of(const node_type& node) const
    {
        return index_of(&node);
    }

private:
    void flatten(const node_type* node)
    {
        if(node->is_branch())
        {
            m_buffer[m_size++] = node->node0;
            m_buffer[m_size++] = node->node1;

            flatten(node->node0);
            flatten(node->node1);
        }
    }

    std::array<const node_type*, N> m_buffer{};
    std::size_t m_size{};
};

template<std::size_t BitLength>
std::vector<std::uint8_t> compress(const std::uint8_t* data, std::size_t size)
{
    std::array<std::size_t, 0x100> frequencies{};

    for(std::size_t ii = 0; ii < size; ++ii)
    {
        int byte = static_cast<int>(data[ii]);

        if(BitLength == 4)
        {
            ++frequencies[byte & 0xf];
            ++frequencies[(byte >> 4) & 0xf];
        }
        else
        {
            ++frequencies[byte];
        }
    }

    node_allocator<0x400> node_allocator{};

    priority_queue<node_type*, 0x400, node_compare> pq;

    for(std::size_t ii = 0, il = frequencies.size(); ii < il; ++ii)
    {
        if(frequencies[ii])
        {
            pq.push(node_allocator.alloc(std::uint8_t(ii), frequencies[ii]));
        }
    }

    if(pq.size() <= 1)
    {
        // Constant value, make root point to it twice
        node_type* node01 = pq.top();
        pq.pop();
        pq.push(node_allocator.alloc(node01->weight + node01->weight, node01, node01));
    }
    else
    {
        while(pq.size() > 1)
        {
            node_type* node0 = pq.top();
            pq.pop();
            node_type* node1 = pq.top();
            pq.pop();

            pq.push(node_allocator.alloc(node0->weight + node1->weight, node0, node1));
        }
    }

    node_type* root = pq.top();

    std::array<bit_code, 0x100> bit_codes{};
    encode(root, bit_code(), bit_codes);

    std::array<std::uint8_t, 0x400> tree_table{};
    std::size_t tree_table_len = 0;

    flat_node_tree<0x400> nodes(root);

    for(std::size_t ii = 0, il = nodes.size(); ii < il; ++ii)
    {
        const node_type& n = *nodes[ii];

        if(n.is_branch())
        {
            std::size_t offset = (nodes.index_of(n.node0) - nodes.index_of(n)) - 1;
            std::size_t jump = offset / 2;

            if(n.node0->is_leaf())
            {
                jump |= 0x80;
            }

            if(n.node1->is_leaf())
            {
                jump |= 0x40;
            }

            tree_table[tree_table_len++] = std::uint8_t(jump);
        }
        else
        {
            tree_table[tree_table_len++] = n.data;
        }
    }

    std::size_t max_result_length = tree_table_len + (size * 2 * 4) + 4;
    std::vector<std::uint8_t> result(max_result_length);
    std::size_t result_length = 0;

    // Remember where we will write table size
    std::size_t data_offset_cur = result_length++;

    // Write tree_table
    for(std::size_t ii = 0; ii < tree_table_len; ++ii)
    {
        result[result_length++] = tree_table[ii];
    }

    std::vector<bit_code> bit_array;
    std::size_t bit_array_len = 0;

    if(BitLength == 4)
    {
        bit_array.resize(size * 2);
    }
    else
    {
        bit_array.resize(size);
    }

    for(std::size_t ii = 0; ii < size; ++ii)
    {
        int byte = static_cast<int>(data[ii]);

        if(BitLength == 4)
        {
            bit_array[bit_array_len++] = bit_codes[byte & 0xf];
            bit_array[bit_array_len++] = bit_codes[(byte >> 4) & 0xf];
        }
        else
        {
            bit_array[bit_array_len++] = bit_codes[byte];
        }
    }

    result_length = (((result_length + 3) / 4)) * 4;
    result[data_offset_cur] = std::uint8_t((result_length / 2) - 1);

    std::uint32_t word = 0;
    std::size_t word_len = 32;

    for(std::size_t ii = 0; ii < bit_array_len; ++ii)
    {
        const bit_code& bc = bit_array[ii];

        for(std::size_t jj = 0, jl = bc.length; jj < jl; ++jj)
        {
            --word_len;

            if((bc.value >> jj) & 1)
            {
                word |= (1ull << word_len);
            }

            if(word_len == 0)
            {
                result[result_length++] = std::uint8_t(word >> 0);
                result[result_length++] = std::uint8_t(word >> 8);
                result[result_length++] = std::uint8_t(word >> 16);
                result[result_length++] = std::uint8_t(word >> 24);
                word = 0;
                word_len = 32;
            }
        }
    }

    if(word_len < 32)
    {
        result[result_length++] = std::uint8_t(word >> 0);
        result[result_length++] = std::uint8_t(word >> 8);
        result[result_length++] = std::uint8_t(word >> 16);
        result[result_length++] = std::uint8_t(word >> 24);
    }

    result.resize(result_length);
    return result;
}

}

result_type compress_4(const std::uint8_t* data, std::size_t size)
{
    result_type result{};
    result.header.type = bios_uncomp_header::type::huffman_4;
    result.header.length = ((size + 3) >> 2) << 2;
    result.data = detail::compress<4>(data, size);
    return result;
}

result_type compress_8(const std::uint8_t* data, std::size_t size)
{
    result_type result{};
    result.header.type = bios_uncomp_header::type::huffman_8;
    result.header.length = ((size + 3) >> 2) << 2;
    result.data = detail::compress<8>(data, size);
    return result;
}

}

#endif


#include "cprs.h"

uint huffgba_compress(RECORD *dst, const RECORD *src)
{
    if(dst==NULL || src==NULL || src->data==NULL)
        return 0;

    int srcS= src->width*src->height;

    if(srcS < 0)
        return 0;

    gba_huff::result_type huffman_4 = gba_huff::compress_4(src->data, std::size_t(srcS));
    gba_huff::result_type huffman_8 = gba_huff::compress_8(src->data, std::size_t(srcS));
    const gba_huff::result_type* best_huffman = huffman_4.data.size() < huffman_8.data.size() ?
                &huffman_4 : &huffman_8;
    std::size_t data_size = best_huffman->data.size();

    if(data_size == 0)
        return 0;

    std::size_t header_size = sizeof(gba_huff::bios_uncomp_header);
    std::size_t total_size = header_size + data_size;
    dst->data= (BYTE*)malloc(total_size);
    memcpy(dst->data, &best_huffman->header, header_size);
    memcpy(dst->data + header_size, best_huffman->data.data(), data_size);

    dst->width= 1;
    dst->height= int(total_size);

    return total_size;
}

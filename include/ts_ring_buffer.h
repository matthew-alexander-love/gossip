/**
 * GossipSampling
 * Copyright (C) Matthew Love 2024 (gossipsampling@gmail.com)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */


#pragma once

#include <mutex>
#include <vector>

namespace gossip {
    template <class T> 
    class TSRingBuffer{
        private:
            mutable std::mutex _lock;
            T *_data;
            uint32_t _head;
            uint32_t _tail;
            uint32_t _current_size;
            const uint32_t _max_size;

        public:
            TSRingBuffer(uint32_t max_size) : _data(static_cast<T*>(operator new(max_size * sizeof(T)))), 
                                        _head(0), _tail(0), _current_size(0), _max_size(max_size) {}

            TSRingBuffer(const TSRingBuffer<T>& other) = delete; // Don't allow copying with std::mutexes associated.
            
            virtual ~TSRingBuffer() {
                while (_current_size != 0) {
                    pop();
                }
                delete(_data);
            }

            const T& at_idx(uint32_t idx) const {
                std::lock_guard<std::mutex> lock(_lock);
                idx %= _current_size;
                idx += _tail;
                idx %= _max_size;

                return _data[idx];
            }

            const T& front() const {
                std::lock_guard<std::mutex> lock(_lock);
                return _data[_tail];
            }

            void pop() {
                std::lock_guard<std::mutex> lock(_lock);
                _data[_tail].~T();
                _tail += 1;
                _tail %= _max_size;
                _current_size -= 1;
            }

            void push(const T& data) {
                std::lock_guard<std::mutex> lock(_lock);
                if (_current_size == _max_size) {
                    pop();
                }

                new(&_data[_head]) T(data);
                _head += 1;
                _head %= _max_size;
                _current_size += 1;
            }

            T consume() {
                T temp = front();
                pop();
                return temp;
            }

            bool empty() {
                std::lock_guard<std::mutex> lock(_lock);
                return !_current_size;
            }

            uint32_t size() {
                std::lock_guard<std::mutex> lock(_lock);
                return _current_size;
            }

            std::vector<T> data() const {
                std::lock_guard<std::mutex> lock(_lock);
                std::vector<T> result;
                result.reserve(_current_size);

                for (uint32_t i = 0; i < _current_size; ++i) {
                    // Calculate the actual index in the circular buffer
                    uint32_t idx = (_tail + i) % _max_size;
                    result.push_back(_data[idx]); // Copy the element into the vector
                }

                return result;
            }

        // Iterator class
        class Iterator {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = T;
                using difference_type = std::ptrdiff_t;
                using pointer = T*;
                using reference = T&;

                Iterator(const TSRingBuffer* buffer, uint32_t idx)
                    : buffer_(buffer), idx_(idx) {}

                const T& operator*() const {
                    return buffer_->at_idx(idx_);
                }

                Iterator& operator++() { // Prefix increment
                    ++idx_;
                    return *this;
                }

                Iterator operator++(int) { // Postfix increment
                    Iterator temp = *this;
                    ++(*this);
                    return temp;
                }

                bool operator==(const Iterator& other) const {
                    return buffer_ == other.buffer_ && idx_ == other.idx_;
                }

                bool operator!=(const Iterator& other) const {
                    return !(*this == other);
                }

            private:
                const TSRingBuffer* buffer_;
                uint32_t idx_;
        };

        // Begin and end methods for iteration
        Iterator begin() const {
            std::lock_guard<std::mutex> lock(_lock);
            return Iterator(this, 0);
        }

        Iterator end() const {
            std::lock_guard<std::mutex> lock(_lock);
            return Iterator(this, _current_size);
        }
    };
}
/*
 *  Copyright (C) 2018, Throughwave (Thailand) Co., Ltd.
 *  <peerawich at throughwave dot co dot th>
 *
 *  This file is part of libnogdb, the NogDB core library in C++.
 *
 *  libnogdb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __RELATION_ADAPTER_HPP_INCLUDED_
#define __RELATION_ADAPTER_HPP_INCLUDED_

#include <string>
#include <cstdlib>
#include <unordered_map>

#include "constant.hpp"
#include "storage_adapter.hpp"

#include "nogdb/nogdb_txn.h"

namespace nogdb {

    namespace adapter {

        namespace relation {

            enum class Direction { IN, OUT };

            /**
             * Raw record format in lmdb data storage:
             * {vertexId<string>} -> {edgeId<RecordId>}{neighborId<RecordId>}
             */
            struct RelationAccessInfo {
                RecordId vertexId{};
                RecordId edgeId{};
                RecordId neighborId{};
            };

            constexpr char KEY_SEPARATOR = ':';

            class RelationAccess : public storage_engine::adapter::LMDBKeyValAccess {
            public:
                RelationAccess() = default;

                RelationAccess(const storage_engine::LMDBTxn * const txn, const Direction& direction)
                        : LMDBKeyValAccess(txn, (direction == Direction::IN)? TB_RELATIONS_IN: TB_RELATIONS_OUT,
                                           true, false, false, false),
                          _direction{direction} {}

                virtual ~RelationAccess() noexcept = default;

                void create(const RelationAccessInfo& props) {
                    put(rid2str(props.vertexId), convertToBlob(props));
                }

                void remove(const RecordId& vertexId) {
                    del(rid2str(vertexId));
                }

                void remove(const RelationAccessInfo& props) {
                    del(rid2str(props.vertexId), convertToBlob(props));
                }

                //TODO: get by cursor and remove by the current cursor
                //std::vector<RelationAccessInfo> getInfosAndRemove(const RecordId& vertexId) {}

                std::vector<RelationAccessInfo> getInfos(const RecordId& vertexId) const {
                    auto result = std::vector<RelationAccessInfo>{};
                    auto cursorHandler = cursor();
                    for(auto keyValue = cursorHandler.find(rid2str(vertexId));
                        !keyValue.empty();
                        keyValue = cursorHandler.getNext()) {
                        auto key = str2rid(keyValue.key.data.string());
                        if (key != vertexId) break;
                        result.emplace_back(parse(vertexId, keyValue.val.data.blob()));
                    }
                }

                std::vector<RecordId> getEdges(const RecordId& vertexId, const RecordId& neighborId) const {
                    auto result = std::vector<RelationAccessInfo>{};
                    auto cursorHandler = cursor();
                    for(auto keyValue = cursorHandler.find(rid2str(vertexId));
                        !keyValue.empty();
                        keyValue = cursorHandler.getNext()) {
                        auto key = str2rid(keyValue.key.data.string());
                        if (key != vertexId) break;
                        auto neighbor = parseNeighborId(keyValue.val.data.blob());
                        if (neighbor != neighborId) continue;
                        result.emplace_back(parseEdgeId(keyValue.val.data.blob()));
                    }
                }

                Direction getDirection() const {
                    return _direction;
                };

            protected:

                static Blob convertToBlob(const RelationAccessInfo& props) {
                    auto totalLength = 2 * sizeof(ClassId) + 2 * sizeof(PositionId);
                    auto value = Blob(totalLength);
                    value.append(&props.edgeId.first, sizeof(ClassId));
                    value.append(&props.edgeId.second, sizeof(PositionId));
                    value.append(&props.neighborId.first, sizeof(ClassId));
                    value.append(&props.neighborId.second, sizeof(PositionId));
                    return value;
                }

                RelationAccessInfo parse(const RecordId& vertexId, const Blob& blob) const {
                    return RelationAccessInfo{
                        vertexId,
                        parseEdgeId(blob),
                        parseNeighborId(blob)
                    };
                }

                static RecordId parseEdgeId(const Blob& blob) {
                    auto edgeId = RecordId{};
                    blob.retrieve(&edgeId.first, 0, sizeof(ClassId));
                    blob.retrieve(&edgeId.second, sizeof(ClassId), sizeof(PositionId));
                    return edgeId;
                }

                static RecordId parseNeighborId(const Blob& blob) {
                    auto neighborId = RecordId{};
                    auto sizeOfRecordId = sizeof(ClassId) + sizeof(PositionId);
                    blob.retrieve(&neighborId.first, sizeOfRecordId, sizeof(ClassId));
                    blob.retrieve(&neighborId.second, sizeOfRecordId + sizeof(ClassId), sizeof(PositionId));
                    return neighborId;
                }

            private:

                const Direction _direction;

                RecordId str2rid(const std::string& key) const {
                    auto splitKey = utils::string::split(key, KEY_SEPARATOR);
                    require(splitKey.size() == 2);
                    auto classId = ClassId{std::atoi(splitKey[0].c_str())};
                    auto positionId = PositionId{std::strtoul(splitKey[1].c_str(), nullptr, 0)};
                    return RecordId{classId, positionId};
                };

            };
        }

    }

}

#endif //__RELATION_ADAPTER_HPP_INCLUDED_
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

#include <iterator>

#include "schema.hpp"

#include <nogdb.h>
#include "nogdb_errors.h"
#include "nogdb_types.h"

namespace nogdb {

    ResultSetCursor::ResultSetCursor(Txn &txn_)
            : txn{txn_}, currentIndex{-1} {}

    ResultSetCursor::~ResultSetCursor() noexcept {}

    ResultSetCursor::ResultSetCursor(ResultSetCursor &&rc) noexcept: txn{rc.txn} {
        metadata = std::move(rc.metadata);
        currentIndex = rc.currentIndex;
    }

    ResultSetCursor &ResultSetCursor::operator=(ResultSetCursor &&rc) noexcept {
        if (this != &rc) {
            txn = std::move(rc.txn);
            metadata = std::move(rc.metadata);
            currentIndex = rc.currentIndex;
        }
        return *this;
    }

    bool ResultSetCursor::hasNext() const {
        return !(metadata.empty()) && (currentIndex < static_cast<long long>(metadata.size() - 1));
    }

    bool ResultSetCursor::hasPrevious() const {
        return !(metadata.empty()) && (currentIndex > 0);
    }

    bool ResultSetCursor::hasAt(unsigned long index) const {
        return !(metadata.empty()) && (index < metadata.size() - 1);
    }

    bool ResultSetCursor::next() {
        if (!metadata.empty() && (currentIndex == -1)) {
            currentIndex = 0;
        } else if (hasNext()) {
            ++currentIndex;
        } else {
            return false;
        }
        auto cursor = metadata.begin() + currentIndex;
        auto recordDescriptor = *(cursor);
        result = Result{recordDescriptor, DB::getRecord(txn, recordDescriptor)};
        return true;
    }

    bool ResultSetCursor::previous() {
        if (!metadata.empty() && (currentIndex >= static_cast<long long>(metadata.size()))) {
            currentIndex = static_cast<long long>(metadata.size() - 1);
        } else if (hasPrevious()) {
            --currentIndex;
        } else {
            return false;
        }
        auto cursor = metadata.begin() + currentIndex;
        auto recordDescriptor = *(cursor);
        result = Result{recordDescriptor, DB::getRecord(txn, recordDescriptor)};
        return true;
    }

    bool ResultSetCursor::empty() const {
        return metadata.empty();
    }

    size_t ResultSetCursor::size() const {
        return metadata.size();
    }

    size_t ResultSetCursor::count() const {
        return size();
    }

    void ResultSetCursor::first() {
        if (!metadata.empty()) {
            currentIndex = 0;
            auto cursor = metadata.begin();
            auto recordDescriptor = *(cursor);
            result = Result{recordDescriptor, DB::getRecord(txn, recordDescriptor)};
        }
    }

    void ResultSetCursor::last() {
        if (!metadata.empty()) {
            currentIndex = static_cast<long long>(metadata.size() - 1);
            auto cursor = metadata.end() - 1;
            auto recordDescriptor = *(cursor);
            result = Result{recordDescriptor, DB::getRecord(txn, recordDescriptor)};
        }
    }

    bool ResultSetCursor::to(unsigned long index) {
        if (index >= metadata.size()) {
            return false;
        }
        currentIndex = index;
        auto cursor = metadata.begin() + currentIndex;
        auto recordDescriptor = *(cursor);
        result = Result{recordDescriptor, DB::getRecord(txn, recordDescriptor)};
        return true;
    }

    const Result &ResultSetCursor::operator*() const {
        return result;
    }

    const Result *ResultSetCursor::operator->() const {
        return &(operator*());
    }

}
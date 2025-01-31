/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#ifndef IFCENTITYLIST_H
#define IFCENTITYLIST_H

#include "IfcBaseClass.h"

#include <boost/shared_ptr.hpp>
#include <set>

template <class T>
class aggregate_of;

class IFC_PARSE_API aggregate_of_instance {
    std::vector<IfcUtil::IfcBaseClass*> ls;

  public:
    typedef boost::shared_ptr<aggregate_of_instance> ptr;
    typedef std::vector<IfcUtil::IfcBaseClass*>::const_iterator it;
    void push(IfcUtil::IfcBaseClass* l);
    void push(const ptr& l);
    it begin();
    it end();
    IfcUtil::IfcBaseClass* operator[](int i);
    unsigned int size() const;
    void reserve(unsigned capacity);
    bool contains(IfcUtil::IfcBaseClass*) const;
    template <class U>
    typename U::list::ptr as() {
        typename U::list::ptr r(new typename U::list);
        for (it i = begin(); i != end(); ++i) {
            if ((*i)->as<U>()) {
                r->push((*i)->as<U>());
            }
        }
        return r;
    }
    void remove(IfcUtil::IfcBaseClass*);
    aggregate_of_instance::ptr filtered(const std::set<const IfcParse::declaration*>& entities);
    aggregate_of_instance::ptr unique();
};

template <class T>
class aggregate_of {
    std::vector<T*> ls;

  public:
    typedef boost::shared_ptr<aggregate_of<T>> ptr;
    typedef typename std::vector<T*>::const_iterator it;
    void push(T* t) {
        if (t) {
            ls.push_back(t);
        }
    }
    void push(ptr t) {
        if (t) {
            for (typename T::list::it it = t->begin(); it != t->end(); ++it) {
                push(*it);
            }
        }
    }
    it begin() { return ls.begin(); }
    it end() { return ls.end(); }
    unsigned int size() const { return (unsigned int)ls.size(); }
    aggregate_of_instance::ptr generalize() {
        aggregate_of_instance::ptr r(new aggregate_of_instance());
        for (it i = begin(); i != end(); ++i) {
            r->push((*i)->template as<IfcUtil::IfcBaseClass>());
        }
        return r;
    }
    bool contains(T* t) const { return std::find(ls.begin(), ls.end(), t) != ls.end(); }
    template <class U>
    typename U::list::ptr as() {
        typename U::list::ptr r(new typename U::list);
        const bool all = !U::Class().as_entity();
        for (it i = begin(); i != end(); ++i) {
            if (all || (*i)->declaration().is(U::Class())) {
                r->push((U*)*i);
            }
        }
        return r;
    }
    void remove(T* t) {
        typename std::vector<T*>::iterator it;
        while ((it = std::find(ls.begin(), ls.end(), t)) != ls.end()) {
            ls.erase(it);
        }
    }
};

template <class T>
class aggregate_of_aggregate_of;

class IFC_PARSE_API aggregate_of_aggregate_of_instance {
    std::vector<std::vector<IfcUtil::IfcBaseClass*>> ls;

  public:
    typedef boost::shared_ptr<aggregate_of_aggregate_of_instance> ptr;
    typedef std::vector<std::vector<IfcUtil::IfcBaseClass*>>::const_iterator outer_it;
    typedef std::vector<IfcUtil::IfcBaseClass*>::const_iterator inner_it;
    void push(const std::vector<IfcUtil::IfcBaseClass*>& l) {
        ls.push_back(l);
    }
    void push(const aggregate_of_instance::ptr& l) {
        if (l) {
            std::vector<IfcUtil::IfcBaseClass*> li;
            for (std::vector<IfcUtil::IfcBaseClass*>::const_iterator jt = l->begin(); jt != l->end(); ++jt) {
                li.push_back(*jt);
            }
            push(li);
        }
    }
    outer_it begin() const { return ls.begin(); }
    outer_it end() const { return ls.end(); }
    int size() const { return (int)ls.size(); }
    int totalSize() const {
        int accum = 0;
        for (outer_it it = begin(); it != end(); ++it) {
            accum += (int)it->size();
        }
        return accum;
    }
    bool contains(IfcUtil::IfcBaseClass* instance) const {
        for (outer_it it = begin(); it != end(); ++it) {
            const std::vector<IfcUtil::IfcBaseClass*>& inner = *it;
            if (std::find(inner.begin(), inner.end(), instance) != inner.end()) {
                return true;
            }
        }
        return false;
    }
    template <class U>
    typename aggregate_of_aggregate_of<U>::ptr as() {
        typename aggregate_of_aggregate_of<U>::ptr r(new aggregate_of_aggregate_of<U>);
        const bool all = !U::Class().as_entity();
        for (outer_it outer = begin(); outer != end(); ++outer) {
            const std::vector<IfcUtil::IfcBaseClass*>& from = *outer;
            typename std::vector<U*> to;
            for (inner_it inner = from.begin(); inner != from.end(); ++inner) {
                if (all || (*inner)->declaration().is(U::Class())) {
                    to.push_back((U*)*inner);
                }
            }
            r->push(to);
        }
        return r;
    }
};

template <class T>
class aggregate_of_aggregate_of {
    std::vector<std::vector<T*>> ls;

  public:
    typedef typename boost::shared_ptr<aggregate_of_aggregate_of<T>> ptr;
    typedef typename std::vector<std::vector<T*>>::const_iterator outer_it;
    typedef typename std::vector<T*>::const_iterator inner_it;
    void push(const std::vector<T*>& t) { ls.push_back(t); }
    outer_it begin() { return ls.begin(); }
    outer_it end() { return ls.end(); }
    int size() const { return (int)ls.size(); }
    int totalSize() const {
        int accum = 0;
        for (outer_it it = begin(); it != end(); ++it) {
            accum += it->size();
        }
        return accum;
    }
    bool contains(T* t) const {
        for (outer_it it = begin(); it != end(); ++it) {
            const std::vector<T*>& inner = *it;
            if (std::find(inner.begin(), inner.end(), t) != inner.end()) {
                return true;
            }
        }
        return false;
    }
    aggregate_of_aggregate_of_instance::ptr generalize() {
        aggregate_of_aggregate_of_instance::ptr r(new aggregate_of_aggregate_of_instance());
        for (outer_it outer = begin(); outer != end(); ++outer) {
            const std::vector<T*>& from = *outer;
            std::vector<IfcUtil::IfcBaseClass*> to;
            for (inner_it inner = from.begin(); inner != from.end(); ++inner) {
                to.push_back(*inner);
            }
            r->push(to);
        }
        return r;
    }
};

#endif

/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/parser/SymbolTable.h"

#include <mutex>
#include <string>
#include <vector>

namespace parsers {

//----------------- Symbol
//---------------------------------------------------------------------------------------------

Symbol::Symbol(std::string const &aName) : name(aName) {}

Symbol::~Symbol() {}

void Symbol::clear() {}

void Symbol::setParent(Symbol *parent_) { this->parent = parent_; }

Symbol *Symbol::getRoot() const {
  Symbol *run = parent;
  while (run != nullptr) {
    if (run->parent == nullptr ||
        dynamic_cast<SymbolTable *>(run->parent) != nullptr)
      return run;
    run = run->parent;
  }
  return run;
}

std::vector<Symbol const *> Symbol::getSymbolPath() const {
  std::vector<Symbol const *> result;
  const Symbol *run = this;
  while (run != nullptr) {
    result.push_back(run);
    if (run->parent == nullptr ||
        dynamic_cast<SymbolTable *>(run->parent) != nullptr)
      break;
    run = run->parent;
  }
  return result;
}

std::string Symbol::qualifiedName(std::string const &separator,
                                  bool full) const {
  std::string result = name;
  Symbol *run = parent;
  while (run != nullptr) {
    result = run->name + separator + result;
    if (!full || run->parent == nullptr ||
        dynamic_cast<SymbolTable *>(run->parent) != nullptr)
      break;
    run = run->parent;
  }
  return result;
}

//----------------- TypedSymbol
//----------------------------------------------------------------------------------------

TypedSymbol::TypedSymbol(std::string const &name_, Type const *aType)
    : Symbol(name_), type(aType) {}

//----------------- ScopedSymbol
//---------------------------------------------------------------------------------------

ScopedSymbol::ScopedSymbol(std::string const &name_) : Symbol(name_) {}

void ScopedSymbol::clear() { children.clear(); }

void ScopedSymbol::addAndManageSymbol(Symbol *symbol) {
  children.emplace_back(symbol);
  symbol->setParent(this);
}

Symbol *ScopedSymbol::resolve(std::string const &name_, bool localOnly) {
  for (auto &child : children) {
    if (child->name == name_) return child.get();
  }

  // Nothing found locally. Let the parent continue.
  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) return scopedParent->resolve(name_, true);
  }

  return nullptr;
}

std::vector<TypedSymbol *> ScopedSymbol::getTypedSymbols(bool localOnly) const {
  std::vector<TypedSymbol *> result = getSymbolsOfType<TypedSymbol>();

  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) {
      auto localList = scopedParent->getTypedSymbols(true);
      result.insert(result.end(), localList.begin(), localList.end());
    }
  }

  return result;
}

std::vector<std::string> ScopedSymbol::getTypedSymbolNames(
    bool localOnly) const {
  std::vector<std::string> result;

  for (auto &child : children) {
    TypedSymbol *typedChild = dynamic_cast<TypedSymbol *>(child.get());
    if (typedChild != nullptr) result.push_back(typedChild->name);
  }

  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) {
      auto localList = scopedParent->getTypedSymbolNames(true);
      result.insert(result.end(), localList.begin(), localList.end());
    }
  }

  return result;
}

std::vector<Type *> ScopedSymbol::getTypes(bool localOnly) const {
  std::vector<Type *> result = getSymbolsOfType<Type>();

  if (!localOnly) {
    ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
    if (scopedParent != nullptr) {
      auto localList = scopedParent->getTypes(true);
      result.insert(result.end(), localList.begin(), localList.end());
    }
  }

  return result;
}

std::vector<ScopedSymbol *> ScopedSymbol::getDirectScopes() const {
  return getSymbolsOfType<ScopedSymbol>();
}

std::vector<Symbol *> ScopedSymbol::getAllSymbols() const {
  std::vector<Symbol *> result;

  for (auto &child : children) {
    result.push_back(child.get());
  }

  ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
  if (scopedParent != nullptr) {
    auto localList = scopedParent->getAllSymbols();
    result.insert(result.end(), localList.begin(), localList.end());
  }

  return result;
}

std::set<std::string> ScopedSymbol::getAllSymbolNames() const {
  std::set<std::string> result;

  for (auto &child : children) {
    result.insert(child->name);
  }

  ScopedSymbol *scopedParent = dynamic_cast<ScopedSymbol *>(parent);
  if (scopedParent != nullptr) {
    auto localList = scopedParent->getAllSymbolNames();
    result.insert(localList.begin(), localList.end());
  }

  return result;
}

//----------------- VariableSymbol
//-------------------------------------------------------------------------------------

VariableSymbol::VariableSymbol(std::string const &name_, Type const *type_)
    : TypedSymbol(name_, type_) {}

//----------------- ParameterSymbol
//------------------------------------------------------------------------------------

//----------------- RoutineSymbol
//--------------------------------------------------------------------------------------

RoutineSymbol::RoutineSymbol(std::string const &name_, Type const *aReturnType)
    : ScopedSymbol(name_), returnType(aReturnType) {}

std::vector<VariableSymbol *> RoutineSymbol::getVariables(bool) const {
  return getSymbolsOfType<VariableSymbol>();
}

std::vector<ParameterSymbol *> RoutineSymbol::getParameters(bool) const {
  return getSymbolsOfType<ParameterSymbol>();
}

//----------------- MethodSymbol
//---------------------------------------------------------------------------------------

MethodSymbol::MethodSymbol(std::string const &name_, Type const *return_type)
    : RoutineSymbol(name_, return_type) {}

//----------------- FieldSymbol
//----------------------------------------------------------------------------------------

FieldSymbol::FieldSymbol(std::string const &name_, Type const *type_)
    : VariableSymbol(name_, type_) {}

//----------------- Type
//-----------------------------------------------------------------------------------------------

Type::Type(std::string const &name_, Type const *base)
    : name(name_), baseType(base) {}

//----------------- FundamentalType
//------------------------------------------------------------------------------------

const Type *FundamentalType::INTEGER_TYPE =
    new FundamentalType("int", TypeKind::Integer);
const Type *FundamentalType::FLOAT_TYPE =
    new FundamentalType("float", TypeKind::Float);
const Type *FundamentalType::STRING_TYPE =
    new FundamentalType("string", TypeKind::String);
const Type *FundamentalType::BOOL_TYPE =
    new FundamentalType("bool", TypeKind::Bool);
const Type *FundamentalType::DATE_TYPE =
    new FundamentalType("date", TypeKind::Date);

FundamentalType::FundamentalType(std::string const &name_, TypeKind kind_)
    : Type(name_), kind(kind_) {}

//----------------- ClassSymbol
//----------------------------------------------------------------------------------------

ClassSymbol::ClassSymbol(std::string const &name_, ClassSymbol *aSuperClass)
    : ScopedSymbol(name_), Type(name_), superClasses({aSuperClass}) {}

std::vector<MethodSymbol *> ClassSymbol::getMethods(bool) const {
  return getSymbolsOfType<MethodSymbol>();
}

std::vector<FieldSymbol *> ClassSymbol::getFields(bool) const {
  return getSymbolsOfType<FieldSymbol>();
}

//----------------- ArrayType
//------------------------------------------------------------------------------------------

ArrayType::ArrayType(std::string const &name_, Type const *elemType,
                     size_t aSize)
    : Type(name_), elementType(elemType), size(aSize) {}

//----------------- TypeAlias
//------------------------------------------------------------------------------------------

TypeAlias::TypeAlias(std::string const &name_, Type const *target)
    : Type(name_, target) {}

//----------------- SymbolTable
//----------------------------------------------------------------------------------------

class SymbolTable::Private {
 public:
  std::recursive_mutex
      mutex;  // Must be in private class for use in C++/CLI code.
};

//----------------------------------------------------------------------------------------------------------------------

SymbolTable::SymbolTable() : ScopedSymbol() { _d = new Private(); }

//----------------------------------------------------------------------------------------------------------------------

SymbolTable::~SymbolTable() { delete _d; }

//----------------------------------------------------------------------------------------------------------------------

void SymbolTable::lock() { _d->mutex.lock(); }

//----------------------------------------------------------------------------------------------------------------------

void SymbolTable::unlock() { _d->mutex.unlock(); }

//----------------------------------------------------------------------------------------------------------------------

void SymbolTable::addDependencies(SymbolTable *dependencies) {
  _dependencies.emplace_back(dependencies);
}

//----------------------------------------------------------------------------------------------------------------------

void SymbolTable::addDependencies(
    std::vector<SymbolTable *> const &newDependencies) {
  // No duplicate check takes place.
  _dependencies.insert(_dependencies.end(), newDependencies.begin(),
                       newDependencies.end());
}

//----------------------------------------------------------------------------------------------------------------------

void SymbolTable::clearDependencies() { _dependencies.clear(); }

//----------------------------------------------------------------------------------------------------------------------

bool SymbolTable::empty() const {
  return _dependencies.empty() && children.empty();
}

//----------------------------------------------------------------------------------------------------------------------

Symbol *SymbolTable::resolve(std::string const &name_, bool localOnly) {
  lock();
  Symbol *result = ScopedSymbol::resolve(name_, localOnly);

  if (result == nullptr && !localOnly) {
    for (auto dependency : _dependencies) {
      result = dependency->resolve(name_, false);
      if (result != nullptr) break;
    }
  }

  unlock();

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

}  // namespace parsers

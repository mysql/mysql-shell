/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/bignum.h"

#include <stdexcept>

#include <openssl/bn.h>
#include <openssl/err.h>

namespace shcore {

namespace {

// thread-scoped context variable which helps in bignum arithmetic
thread_local std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> g_bn_ctx = {
    BN_CTX_new(), &BN_CTX_free};

}  // namespace

class Bignum::Impl {
 public:
  explicit Impl(uint64_t d) : m_bn(convert(d)) {}

  explicit Impl(int64_t d) {
    if (d >= 0) {
      m_bn = convert(static_cast<uint64_t>(d));
    } else {
      m_bn = convert(static_cast<uint64_t>(-d));
      BN_set_negative(m_bn.get(), 1);
    }
  }

  explicit Impl(const std::string &d) {
    BIGNUM *bn = nullptr;

    if (!BN_dec2bn(&bn, d.c_str())) {
      throw_exception("BN_dec2bn() failed");
    }

    m_bn = create(bn);
  }

  Impl(const Impl &other) { *this = other; }

  Impl(Impl &&other) = default;

  Impl &operator=(const Impl &other) {
    m_bn = create(BN_dup(other.m_bn.get()));

    return *this;
  }

  Impl &operator=(Impl &&other) = default;

  ~Impl() = default;

  std::string to_string() const {
    const std::unique_ptr<char, void (*)(char *)> dec{BN_bn2dec(m_bn.get()),
                                                      [](char *str) {
                                                        // this is a macro,
                                                        // cannot be used
                                                        // directly in
                                                        // std::unique_ptr
                                                        OPENSSL_free(str);
                                                      }};
    return dec.get();
  }

  Impl &neg() {
    BN_set_negative(m_bn.get(), !BN_is_negative(m_bn.get()));

    return *this;
  }

  Impl &add(const Impl &rhs) {
    // result can be stored directly in one of the operands
    if (!BN_add(m_bn.get(), m_bn.get(), rhs.m_bn.get())) {
      throw_exception("BN_add() failed");
    }

    return *this;
  }

  Impl &add(BN_ULONG rhs) {
    if (!BN_add_word(m_bn.get(), rhs)) {
      throw_exception("BN_add_word() failed");
    }

    return *this;
  }

  Impl &sub(const Impl &rhs) {
    auto result = create();

    if (!BN_sub(result.get(), m_bn.get(), rhs.m_bn.get())) {
      throw_exception("BN_sub() failed");
    }

    m_bn = std::move(result);

    return *this;
  }

  Impl &sub(BN_ULONG rhs) {
    if (!BN_sub_word(m_bn.get(), rhs)) {
      throw_exception("BN_sub_word() failed");
    }

    return *this;
  }

  Impl &mul(const Impl &rhs) {
    // result can be stored directly in one of the operands
    if (!BN_mul(m_bn.get(), m_bn.get(), rhs.m_bn.get(), g_bn_ctx.get())) {
      throw_exception("BN_mul() failed");
    }

    return *this;
  }

  Impl &div(const Impl &rhs) {
    auto result = create();

    if (!BN_div(result.get(), nullptr, m_bn.get(), rhs.m_bn.get(),
                g_bn_ctx.get())) {
      throw_exception("BN_div() failed");
    }

    m_bn = std::move(result);

    return *this;
  }

  Impl &mod(const Impl &rhs) {
    auto result = create();

    if (!BN_mod(result.get(), m_bn.get(), rhs.m_bn.get(), g_bn_ctx.get())) {
      throw_exception("BN_mod() failed");
    }

    m_bn = std::move(result);

    return *this;
  }

  Impl &lshift(int n) {
    // result can be stored directly in the input argument
    if (!BN_lshift(m_bn.get(), m_bn.get(), n)) {
      throw_exception("BN_lshift() failed");
    }

    return *this;
  }

  Impl &rshift(int n) {
    // result can be stored directly in the input argument
    if (!BN_rshift(m_bn.get(), m_bn.get(), n)) {
      throw_exception("BN_rshift() failed");
    }

    return *this;
  }

  Impl &exp(uint64_t p) {
    Impl power{p};
    auto r = create();

    if (!BN_exp(r.get(), m_bn.get(), power.m_bn.get(), g_bn_ctx.get())) {
      throw_exception("BN_exp() failed");
    }

    m_bn = std::move(r);

    return *this;
  }

  inline int compare(const Impl &rhs) const {
    return BN_cmp(m_bn.get(), rhs.m_bn.get());
  }

  void swap(Impl &rhs) { BN_swap(m_bn.get(), rhs.m_bn.get()); }

 private:
  struct Deleter {
    void operator()(BIGNUM *bn) const { BN_free(bn); }
  };

  using BIGNUM_t = std::unique_ptr<BIGNUM, Deleter>;

  static BIGNUM_t create() { return create(BN_new()); }

  static BIGNUM_t create(BIGNUM *bn) { return BIGNUM_t{bn}; }

  static BIGNUM_t convert(uint64_t d) {
    auto bn = create();

#if 8 == BN_BYTES
    if (!BN_set_word(bn.get(), d)) {
      throw_exception("BN_set_word() failed");
    }
#elif 4 == BN_BYTES
    if (!BN_set_word(bn.get(), static_cast<uint32_t>(d >> 32))) {
      throw_exception("BN_set_word() failed");
    }

    // result can be stored directly in the input argument
    if (!BN_lshift(bn.get(), bn.get(), 32)) {
      throw_exception("BN_lshift() failed");
    }

    if (!BN_add_word(bn.get(), static_cast<uint32_t>(d))) {
      throw_exception("BN_add_word() failed");
    }
#else
#error "Unsupported number of bytes in BN"
#endif

    return bn;
  }

  static void throw_exception(const std::string &context) {
    std::string msg = context + ": " + ERR_reason_error_string(ERR_get_error());
    throw std::runtime_error(msg);
  }

  BIGNUM_t m_bn;
};

Bignum::Bignum(uint64_t d) : m_impl(std::make_unique<Impl>(d)) {}

Bignum::Bignum(int64_t d) : m_impl(std::make_unique<Impl>(d)) {}

Bignum::Bignum(const std::string &d) : m_impl(std::make_unique<Impl>(d)) {}

Bignum::Bignum(const Bignum &other) { *this = other; }

Bignum::Bignum(Bignum &&other) = default;

Bignum &Bignum::operator=(const Bignum &other) {
  m_impl = std::make_unique<Impl>(*other.m_impl);
  return *this;
}

Bignum &Bignum::operator=(Bignum &&other) = default;

Bignum::~Bignum() = default;

std::string Bignum::to_string() const { return m_impl->to_string(); }

Bignum &Bignum::exp(uint64_t p) {
  m_impl->exp(p);
  return *this;
}

Bignum Bignum::operator+() const { return *this; }

Bignum Bignum::operator-() const {
  auto copy = *this;
  copy.m_impl->neg();
  return copy;
}

Bignum &Bignum::operator++() {
  m_impl->add(1);
  return *this;
}

Bignum Bignum::operator++(int) {
  auto copy = *this;
  operator++();
  return copy;
}

Bignum &Bignum::operator--() {
  m_impl->sub(1);
  return *this;
}

Bignum Bignum::operator--(int) {
  auto copy = *this;
  operator--();
  return copy;
}

Bignum &Bignum::operator+=(const Bignum &rhs) {
  m_impl->add(*rhs.m_impl);
  return *this;
}

Bignum &Bignum::operator-=(const Bignum &rhs) {
  m_impl->sub(*rhs.m_impl);
  return *this;
}

Bignum &Bignum::operator*=(const Bignum &rhs) {
  m_impl->mul(*rhs.m_impl);
  return *this;
}

Bignum &Bignum::operator/=(const Bignum &rhs) {
  m_impl->div(*rhs.m_impl);
  return *this;
}

Bignum &Bignum::operator%=(const Bignum &rhs) {
  m_impl->mod(*rhs.m_impl);
  return *this;
}

Bignum &Bignum::operator<<=(int n) {
  m_impl->lshift(n);
  return *this;
}

Bignum &Bignum::operator>>=(int n) {
  m_impl->rshift(n);
  return *this;
}

int Bignum::compare(const Bignum &rhs) const {
  return m_impl->compare(*rhs.m_impl);
}

void Bignum::swap(Bignum &rhs) { m_impl->swap(*rhs.m_impl); }

}  // namespace shcore

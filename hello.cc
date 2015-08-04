/* Copyright (C) 2015  Florian Hassanen
 *
 * This file is part of pint.
 *
 * pint is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pint is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <string>
#include <list>
#include <memory>

using std::begin;
using std::end;

template <typename T> class list_head;

template <typename T> class list_elem {
public:
  list_elem(T &&v, std::shared_ptr<list_elem<T>> n) noexcept(
      noexcept(decltype(value)(std::forward<T>(v))) &&
      noexcept(decltype(next)(n)))
      : value(std::forward<T>(v)), next(n) {}
  list_elem() = delete;
  list_elem(list_elem<T> const &) = default;
  list_elem(list_elem<T> &&) = default;
  list_elem &operator=(list_elem<T> const &) = default;
  list_elem &operator=(list_elem<T> &&) = default;

private:
  T value;
  std::shared_ptr<list_elem<T>> next;

  friend class list_head<T>;
};

template <typename T>
typename list_head<T>::iterator begin(list_head<T> const &);
template <typename T> typename list_head<T>::iterator end(list_head<T> const &);

template <typename T> class list_head {
public:
  list_head() = default;
  list_head(list_head<T> const &) = default;
  list_head(list_head<T> &&) = default;
  list_head &operator=(list_head<T> const &) = default;
  list_head &operator=(list_head<T> &&) = default;
  template <typename... Args> void emplace_front(Args &&... args) {
    head = std::make_shared<list_elem<T>>(T(std::forward<Args>(args)...), head);
  }
  bool empty() const { return head == nullptr; }
  T const front() const { return head->value; }
  T front() {
    return const_cast<T>(const_cast<list_head<T> const *>(this)->front());
  }
  list_head<T> const tail() const { return list_head<T>(head->next); }
  list_head<T> tail() {
    return const_cast<list_head<T>>(
        const_cast<list_head<T> const *>(this)->tail());
  }
  class iterator : public std::iterator<std::forward_iterator_tag, T> {
  public:
    iterator(std::shared_ptr<list_elem<T>> p) noexcept(
        noexcept(decltype(pos)(p)))
        : pos(p) {}
    iterator() = delete;
    iterator(iterator const &) = default;
    iterator(iterator &&) = default;
    iterator &operator=(iterator const &) = default;
    iterator &operator=(iterator &&) = default;
    iterator &operator++() {
      advance();
      return *this;
    }
    iterator operator++(int) {
      auto p = *this;
      advance();
      return p;
    }
    bool operator==(iterator const &other) const {
      return this->pos == other.pos;
    }
    bool operator!=(iterator const &other) const {
      return this->pos != other.pos;
    }
    T const &operator*() const { return this->pos->value; }
    T const *operator->() const { return &this->pos->value; }

  private:
    void advance() {
      if (pos != nullptr) {
        pos = pos->next;
      }
    }
    std::shared_ptr<list_elem<T>> pos;
  };

private:
  list_head(std::shared_ptr<list_elem<T>> h) noexcept(
      noexcept(decltype(head)(h)))
      : head(h) {}
  std::shared_ptr<list_elem<T>> head;

  friend list_head<T>::iterator begin<T>(list_head<T> const &l);
  friend list_head<T>::iterator end<T>(list_head<T> const &l);
};

template <typename T>
typename list_head<T>::iterator begin(list_head<T> const &l) {
  return typename list_head<T>::iterator(l.head);
}
template <typename T>
typename list_head<T>::iterator end(list_head<T> const &) {
  return typename list_head<T>::iterator(nullptr);
}

// value = programmatic value
//       = expr (everything that is code)
//       | function (concrete function value)

class Value {
public:
  virtual ~Value() = default;
};

// expr = list | number | text | symbol
// list = ( expr * ) // todo should be "value *"

class Expr : public Value {};

template <typename T> class ValueHolder {
public:
  ValueHolder() = default;
  ValueHolder(T const &o) noexcept(noexcept(decltype(v)(o))) : v(o) {}
  ValueHolder(T &&o) noexcept(noexcept(decltype(v)(std::move(o))))
      : v(std::move(o)) {}
  T const &value() const { return v; }

private:
  T v;
};

class Number : public Expr, public ValueHolder<double> {
  using ValueHolder::ValueHolder;
};

class Text : public Expr, public ValueHolder<std::string> {
  using ValueHolder::ValueHolder;
};

class Symbol : public Expr, public ValueHolder<std::string> {
  using ValueHolder::ValueHolder;
};

class List : public Expr,
             public ValueHolder<list_head<std::shared_ptr<Value>>> {
  using ValueHolder::ValueHolder;
};

// todo: shared ptr correct choice?
class ParseResult {
public:
  ParseResult(std::shared_ptr<Expr> expr,
              std::size_t pos) noexcept(noexcept(decltype(expr_)(expr)) &&
                                        noexcept(decltype(pos_)(pos)))
      : expr_(expr), pos_(pos) {}
  std::size_t pos() const { return pos_; }
  bool parsed() const { return expr_ != nullptr; }
  std::shared_ptr<Expr> result() const { return expr_; }

private:
  std::shared_ptr<Expr> expr_;
  std::size_t pos_;
};

std::string const newline = "\n\r";
std::string const whitespace = newline + " \t";
std::string const digit = "1234567890";

ParseResult parse_expr(std::string const &s, std::size_t pos);

std::size_t len(std::size_t start, std::size_t end) {
  if (end == std::string::npos) {
    return end;
  }
  return end - start;
}

ParseResult parse_symbol(std::string const &s, std::size_t pos) {
  auto f = s.find_first_of(whitespace + "()'", pos);

  std::string sym(s, pos, len(pos, f));

  std::cout << sym << std::endl;

  return ParseResult(sym.length() ? std::make_shared<Symbol>(sym) : nullptr, f);
}

ParseResult parse_string(std::string const &s, std::size_t pos) {
  ++pos;
  std::string ret;
  while (true) {
    auto f = s.find_first_of("'\\", pos);

    ret += std::string(s, pos, len(pos, f));

    if (s[f] == '\\') {
      if (++f < s.size()) { // copy next char as is
        ret += s[f];
      }
      pos = ++f; // skip behind escape
      continue;
    }

    if (s[f] == '\'') {
      std::cout << "'" << ret << "'" << std::endl;
      return ParseResult(std::make_shared<Text>(ret), ++f);
    }

    return ParseResult(nullptr, pos);
  }
}

ParseResult parse_number(std::string const &s, std::size_t pos) {
  auto f = s.find_first_not_of(digit, pos);

  if (f != std::string::npos && s[f] == '.') {
    f = s.find_first_not_of(digit, f + 1);
  }

  if (digit.find(s[f - 1]) == std::string::npos) {
    return ParseResult(nullptr, pos);
  }

  double num = std::stod(std::string(s, pos, len(pos, f)));

  std::cout << num << std::endl;

  return ParseResult(std::make_shared<Number>(num), f);
}

#include <algorithm>

ParseResult parse_list(std::string const &s, std::size_t pos) {
  std::list<std::shared_ptr<Value>> l;

  std::cout << "(" << std::endl;

  ++pos;
  while (true) {
    ParseResult r = parse_expr(s, pos);

    if (r.parsed()) {
      std::cout << "." << std::endl;

      pos = r.pos();
      l.emplace_back(r.result());

      continue;
    }

    if (r.pos() == std::string::npos || s[r.pos()] != ')') {
      return ParseResult(nullptr, r.pos());
    }

    std::cout << ")" << std::endl;

    std::reverse(begin(l), end(l));

    list_head<std::shared_ptr<Value>> ll;

    for (auto const &e : l) {
      ll.emplace_front(e);
    }

    return ParseResult(std::make_shared<List>(ll), r.pos() + 1);
  }
}

ParseResult parse_expr(std::string const &s, std::size_t pos) {
  auto f = s.find_first_not_of(whitespace, pos);
  if (f == std::string::npos) {
    return ParseResult(nullptr, pos);
  } else if (s[f] == '(') {
    return parse_list(s, f);
  } else if (s[f] == '\'') {
    return parse_string(s, f);
  } else if (digit.find(s[f]) != std::string::npos) {
    return parse_number(s, f);
  } else if (s[f] == ';') {
    return parse_expr(s, s.find_first_of(newline, f + 1));
  } else {
    return parse_symbol(s, f);
  }
}

#include <unordered_map>

// function = arg_binding + var_context + logic

// simple mode create function objects during eval
using FunctionType = std::function<std::shared_ptr<Value>(
    std::list<std::shared_ptr<Value>> const &)>;
class Function : public Value, public ValueHolder<FunctionType> {
  using ValueHolder::ValueHolder;
};
class Macro : public Function {
  using Function::Function;
};

using Memory = std::unordered_map<std::string, std::shared_ptr<Value>>;
Memory root;

#include <sstream>

void print(Value const *value) {
  if (auto d = dynamic_cast<Number const *>(value)) {
    std::cout << d->value();
  } else if (auto t = dynamic_cast<Text const *>(value)) {
    std::cout << t->value();
  } else if (auto s = dynamic_cast<Symbol const *>(value)) {
    std::cout << "[Symbol " << s->value() << "]";
  } else if (auto li = dynamic_cast<List const *>(value)) {
    std::cout << "(";
    std::string sep = "";
    for (auto const &vv : li->value()) {
      std::cout << sep;
      sep = " ";
      print(vv.get());
    }
    std::cout << ")";
  } else if (dynamic_cast<Function const *>(value)) {
    std::cout << "[Function]";
  } else {
    std::cout << "[nil]";
  }
}

std::string to_string(Value const *value) {
  if (auto d = dynamic_cast<Number const *>(value)) {
    std::ostringstream oss;
    oss << d->value();
    return oss.str();
  } else if (auto t = dynamic_cast<Text const *>(value)) {
    return t->value();
  } else if (auto s = dynamic_cast<Symbol const *>(value)) {
    return s->value();
  } else if (auto li = dynamic_cast<List const *>(value)) {
    std::string ret = "";
    for (auto const &vv : li->value()) {
      ret += to_string(vv.get());
    }
    return ret;
  } else {
    return "";
  }
}

bool eq(std::shared_ptr<Value> const &l, std::shared_ptr<Value> const &r) {
  if (l == r) {
    // fns are never eq
    return !dynamic_cast<Function *>(l.get());
  }

  if (auto nl = dynamic_cast<Number *>(l.get())) {
    if (auto nr = dynamic_cast<Number *>(r.get())) {
      return nl->value() == nr->value();
    }
    return false;
  }

  if (auto tl = dynamic_cast<Text *>(l.get())) {
    if (auto tr = dynamic_cast<Text *>(r.get())) {
      return tl->value() == tr->value();
    }
    return false;
  }

  if (auto sl = dynamic_cast<Symbol *>(l.get())) {
    if (auto sr = dynamic_cast<Symbol *>(r.get())) {
      return sl->value() == sr->value();
    }
    return false;
  }

  if (auto ll = dynamic_cast<List *>(l.get())) {
    if (auto lr = dynamic_cast<List *>(r.get())) {
      auto const &lll = ll->value();
      auto const &llr = lr->value();
      return std::equal(begin(lll), end(lll), begin(llr), end(llr), eq);
    }
    return false;
  }
  return false;
}

void setup() {
  root["print"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          print(v.get());
        }
        return nullptr;
      });
  root["str"] = std::make_shared<Function>([](auto const &l) {
    auto rl = l;
    std::reverse(begin(rl), end(rl));
    list_head<std::shared_ptr<Value>> ll;
    for (auto const &v : rl) {
      ll.emplace_front(v);
    }
    List lll(ll);
    return std::make_shared<Text>(to_string(&lll));
  });
  root["substr"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        auto i = begin(l);
        auto e = end(l);

        if (i != e) {
          if (auto t = dynamic_cast<Text *>(i->get())) {
            if (++i == e) {
              return l.front();
            } else {
              if (auto pos = dynamic_cast<Number *>(i->get())) {
                if (++i == e) {
                  return std::make_shared<Text>(std::string(
                      t->value(), static_cast<std::size_t>(pos->value())));
                } else if (auto len = dynamic_cast<Number *>(i->get())) {
                  return std::make_shared<Text>(std::string(
                      t->value(), static_cast<std::size_t>(pos->value()),
                      static_cast<std::size_t>(len->value())));
                }
              }
            }
          }
        }
        return nullptr;
      });

  root["symbol-name"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto sym = dynamic_cast<Symbol *>(v.get())) {
            return std::make_shared<Text>(sym->value());
          }
          break;
        }
        return nullptr;
      });
  root["symbol"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto tex = dynamic_cast<Text *>(v.get())) {
            return std::make_shared<Symbol>(tex->value());
          }
          break;
        }
        return nullptr;
      });
  root["cons"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        auto i = begin(l);
        auto e = end(l);

        if (i == e) {
          return nullptr;
        }

        auto head = *i;

        if (++i == e || *i == nullptr) {
          list_head<std::shared_ptr<Value>> nl;
          nl.emplace_front(head);
          return std::make_shared<List>(nl);
        }

        if (auto ol = dynamic_cast<List *>(i->get())) {
          list_head<std::shared_ptr<Value>> nl(ol->value());
          nl.emplace_front(head);
          return std::make_shared<List>(nl);
        } else {
          return nullptr;
        }
      });
  root["head"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto ll = dynamic_cast<List *>(v.get())) {
            if (!ll->value().empty()) {
              return ll->value().front();
            }
          }
          break;
        }
        return nullptr;
      });
  root["tail"] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        for (auto const &v : l) {
          if (auto ll = dynamic_cast<List *>(v.get())) {
            if (!ll->value().empty()) {
              return std::make_shared<List>(ll->value().tail());
            }
          }
          break;
        }
        return nullptr;
      });
  root["+"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s += d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["-"] = std::make_shared<Function>([](auto const &l) {
    double s = 0;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s -= d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["*"] = std::make_shared<Function>([](auto const &l) {
    double s = 1;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s *= d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["/"] = std::make_shared<Function>([](auto const &l) {
    double s = 1;
    for (auto const &v : l) {
      if (Number *d = dynamic_cast<Number *>(v.get())) {
        s /= d->value();
      }
    }
    return std::make_shared<Number>(s);
  });
  root["="] =
      std::make_shared<Function>([](auto const &l) -> std::shared_ptr<Value> {
        std::shared_ptr<Value> const *p = nullptr;
        for (auto const &v : l) {
          if (p && !eq(*p, v)) {
            return nullptr;
          }
          p = &v;
        }
        return p == nullptr ? std::make_shared<Text>("") : *p;
      });
}

#include <unordered_set>

std::unordered_set<std::string> free_vars(List *l) {
  std::unordered_set<std::string> vars;

  Symbol *s = nullptr;
  List *args = nullptr;
  int i = 0;
  bool is_fn = false;
  for (auto const &e : l->value()) {
    if (i == 0 && (s = dynamic_cast<Symbol *>(e.get())) && "fn" == s->value()) {
      is_fn = true;
    } else if (is_fn && i == 1 && (args = dynamic_cast<List *>(e.get()))) {
    } else if ((s = dynamic_cast<Symbol *>(e.get()))) {
      vars.emplace(s->value());
    } else if (List *il = dynamic_cast<List *>(e.get())) {
      for (auto const &v : free_vars(il)) {
        vars.emplace(v);
      }
    }
    ++i;
  }
  if (args != nullptr) {
    for (auto const &v : free_vars(args)) {
      vars.erase(v);
    }
  }

  return vars;
}

Memory copy_vars(Memory &m, std::unordered_set<std::string> free_vars) {
  Memory ret;

  for (auto const &v : free_vars) {
    ret[v] = m[v];
  }

  return ret;
}

std::list<std::string> arg_list(List const *f) {
  std::list<std::string> ret;

  auto i = begin(f->value());

  ++i;

  for (auto const &a : dynamic_cast<List *>(i->get())->value()) {
    if (Symbol *s = dynamic_cast<Symbol *>(a.get())) {
      ret.emplace_back(s->value());
    }
  }

  return ret;
}

std::list<std::shared_ptr<Value>> body_list(List const *f) {
  std::list<std::shared_ptr<Value>> ret;

  int i = 0;
  for (auto const &e : f->value()) {
    if (i >= 2) {
      ret.emplace_back(e);
    }

    ++i;
  }

  return ret;
}

Memory populate(Memory m, std::list<std::string> args,
                std::list<std::shared_ptr<Value>> const &vals) {
  auto i = begin(vals);
  for (auto const &s : args) {
    m[s] = *i++;
  }
  return m;
}

std::shared_ptr<Value> eval(Memory &, std::shared_ptr<Value>);

FunctionType eval_to_f(Memory &pm, List *f) {
  return [
    m(copy_vars(pm, free_vars(f))),
    args(arg_list(f)),
    body(body_list(f))
  ](auto const &arg_vals) {
    auto mm = populate(m, args, arg_vals);

    std::shared_ptr<Value> r = nullptr;

    for (auto const &e : body) {
      r = eval(mm, e);
    }

    return r;
  };
}

std::shared_ptr<Value> eval(Memory &m, std::shared_ptr<Value> expr) {
  if (auto var = dynamic_cast<Symbol *>(expr.get())) {
    return m[var->value()];
  } else if (auto l = dynamic_cast<List *>(expr.get())) {
    if (l->value().empty()) {
      return std::make_shared<List>(); // empty form
    }
    auto const &list = l->value();
    auto i = begin(list);
    auto e = end(list);
    if (auto s = dynamic_cast<Symbol *>(i->get())) {
      if ("do" == s->value()) {
        std::shared_ptr<Value> r;
        while (++i != e) {
          r = eval(m, *i);
        }
        return r;
      }
      if ("def" == s->value()) {
        if (++i != e) {
          if (auto t = dynamic_cast<Symbol *>(i->get())) {
            if (++i != e) {
              return (m[t->value()] = eval(m, *i));
            }
          }
        }
        return nullptr;
      }
      if ("fn" == s->value()) {
        return std::make_shared<Function>(eval_to_f(m, l));
      }
      if ("macro" == s->value()) {
        return std::make_shared<Macro>(eval_to_f(m, l));
      }
      if ("if" == s->value()) {
        if (++i != e) {
          auto cond = eval(m, *i);
          if (++i != e) {
            if (cond != nullptr) {
              return eval(m, *i);
            } else if (++i != e) {
              return eval(m, *i);
            }
          }
        }
        return nullptr;
      }
      if ("quote" == s->value()) {
        if (++i != e) {
          return *i;
        }
        return nullptr;
      }
    }
    auto head = eval(m, *i);
    if (Macro *c = dynamic_cast<Macro *>(head.get())) {
      std::list<std::shared_ptr<Value>> args;
      while (++i != e) {
        args.emplace_back(*i);
      }
      return eval(m, (c->value())(args));
    }
    if (Function *f = dynamic_cast<Function *>(head.get())) {
      std::list<std::shared_ptr<Value>> args;
      while (++i != e) {
        args.emplace_back(eval(m, *i));
      }
      return (f->value())(args);
    }
    return nullptr;
  } else {
    return expr;
  }
}

#include <fstream>
#include <dlfcn.h>
#include <ffi.h>

class Dl {
public:
  Dl() : dl_handle(dlopen(nullptr, RTLD_LAZY)) {}
  Dl(Dl const &) = delete;
  Dl &operator=(Dl const &) = delete;
  bool valid() const { return dl_handle != nullptr; }
  void *function(std::string name) const {
    return dlsym(dl_handle, name.c_str());
  }
  ~Dl() {
    if (dl_handle != nullptr) {
      dlclose(dl_handle);
    }
  }

private:
  void *dl_handle;
};

int main(int argc, char **argv) {
  Dl dl;
  if (!dl.valid()) {
    std::cout << "fail" << std::endl;
  } else {
    std::cout << "succ" << std::endl;
    void *fp = dl.function("puts");
    ((int (*)(char *))fp)("hmmm...");
    if (fp == nullptr) {
      std::cout << "fail" << std::endl;
    } else {
      std::cout << "succ" << std::endl;
      ffi_cif cif;
      ffi_type *argt[] = {&ffi_type_pointer};
      char *h = "hello from ffi!";
      void *argv[] = {(void *)&h};
      ffi_status st =
          ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_sint, argt);
      if (st != FFI_OK) {
        std::cout << "fail" << std::endl;
      } else {
        std::cout << "succ" << std::endl;
        int frs;
        ffi_call(&cif, (void (*)())fp, &frs, argv);
      }
    }
  }

  setup();
  std::cout << "hello world!" << std::endl;

  std::ostringstream str;

  if (argc >= 2) {
    str << std::ifstream(argv[1]).rdbuf();
  } else {
    str << std::cin.rdbuf();
  }

  std::string x = str.str();

  if (argc >= 3) {
    // TODO this is a cheat, we need proper io
    std::ostringstream input;
    if (std::string("-") == argv[2]) {
      input << std::cin.rdbuf();
    } else {
      input << std::ifstream(argv[2]).rdbuf();
    }
    root["INPUT"] = std::make_shared<Text>(input.str());
  }

  ParseResult pr = parse_expr(x, 0);
  std::list<std::shared_ptr<Expr>> program;
  while (pr.parsed()) {
    program.emplace_back(pr.result());
    pr = parse_expr(x, pr.pos());
  }
  std::shared_ptr<Value> r = nullptr;
  for (auto const &e : program) {
    r = eval(root, e);
  }
  if (auto n = dynamic_cast<Number *>(r.get())) {
    return static_cast<int>(n->value());
  }
  return 0;
}

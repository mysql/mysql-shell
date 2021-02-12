## imports
import pickle
import random
import sys

## constants
shlist = sys.modules["mysqlsh.__shell_python_support__"].List

slices = [
    slice(4, 4),
    slice(2, 7),
    slice(None, 3),
    slice(3, None),
    slice(None, None),
    slice(-3, None),
    slice(None, -3),
    slice(1, -1),
    slice(-4, 9),
    slice(-4, -1),
]

step_slices = [
    slice(None, None, 2),
    slice(2, None, 3),
    slice(3, -2, 2),
    slice(None, None, -1),
    slice(-1, None, -1),
    slice(-1, 1, -1),
]

all_slices = slices + step_slices

magic_state = 0x12345678

debug = False

## helpers
def exception(exc):
    return tuple([item.replace('mysqlsh.__shell_python_support__.List', 'list') if isinstance(item, str) else item for idx, item in enumerate(exc.args)])if exc is not None else exc

def random_list(*args):
    return [random.randint(0, 100000) for _ in range(*args)]

def SETUP(m, *args, **kwargs):
    global expected
    global actual
    global method
    expected = list(*args, **kwargs)
    actual = shlist(*args, **kwargs)
    method = m

def EXECUTE(run_expected, run_actual, desc, run_cleanup=None):
    global expected
    global actual
    global debug
    e = None
    ee = None
    a = None
    ae = None
    if debug:
        print("description:", desc)
        print("expected (type):", type(expected))
        print("actual   (type):", type(actual))
        print("expected (before):", expected)
        print("actual   (before):", actual)
    try:
        e = run_expected()
    except Exception as exc:
        ee = exc
    if run_cleanup is not None:
        run_cleanup()
    try:
        a = run_actual()
    except Exception as exc:
        ae = exc
    if debug:
        print("expected (after) :", expected)
        print("actual   (after) :", actual)
        print("expected result  :", e)
        print("actual result    :", a)
        print("expected exception:", ee)
        print("actual exception  :", ae)
    EXPECT_EQ(e, a, "result of: " + desc)
    EXPECT_EQ(exception(ee), exception(ae), "exception of: " + desc)
    EXPECT_EQ(expected, actual, "state after: " + desc)

def TEST(*args, **kwargs):
    global expected
    global actual
    global method
    desc  = "L." + method + "("
    desc += ", ".join([str(a) for a in args])
    desc += (", " if len(args) > 0 and len(kwargs) > 0 else "")
    desc += ", ".join([k + "=" + str(v) for k, v in kwargs.items()])
    desc += ")"
    EXECUTE(lambda: expected.__getattribute__(method)(*args, **kwargs), lambda: actual.__getattribute__(method)(*args, **kwargs), desc)

def TEST_LIST_ARG(init, *args, **kwargs):
    global expected
    global actual
    e = expected.copy()
    a = actual.copy()
    for l in [ list, shlist ]:
        expected = e.copy()
        actual = a.copy()
        TEST(*args, l(init), **kwargs)

def TEST_OP(v):
    global expected
    global actual
    global method
    # try to compile to check if string we're going to use is an expression or
    # a compound statement, use eval() or exec() respectively
    try:
        compile(f"expected {method} v", "stdin", "eval")
        func = eval
    except SyntaxError:
        func = exec
    backup = v
    cleanup = lambda: exec("v = backup", g, l)
    g = globals()
    l = locals()
    if debug:
        print("using:", func)
    EXECUTE(lambda: func(f"expected {method} v", g, l), lambda: func(f"actual {method} v", g, l), f"list {method} {str(v)}", cleanup)
    EXECUTE(lambda: func(f"v {method} expected", g, l), lambda: func(f"v {method} actual", g, l), f"{str(v)} {method} list", cleanup)
    EXECUTE(lambda: func(f"expected {method} expected"), lambda: func(f"actual {method} actual"), f"list {method} list", cleanup)

def TEST_OP_LIST_ARG(*args, **kwargs):
    global expected
    global actual
    e = expected.copy()
    a = actual.copy()
    for l in [ list, shlist ]:
        expected = e.copy()
        actual = a.copy()
        TEST_OP(l(*args, **kwargs))

#@<> docs
EXPECT_EQ("L.__getitem__(y) <==> L[y]", shlist.__getitem__.__doc__)
EXPECT_EQ("L.__reversed__() -- return a reverse iterator over the list", shlist.__reversed__.__doc__)
EXPECT_EQ("L.__sizeof__() -- size of L in memory, in bytes", shlist.__sizeof__.__doc__)
EXPECT_EQ("L.append(object) -> None -- append object to end", shlist.append.__doc__)
EXPECT_EQ("L.clear() -> None -- remove all elements from the list", shlist.clear.__doc__)
EXPECT_EQ("L.copy() -> list -- a shallow copy of L", shlist.copy.__doc__)
EXPECT_EQ("L.count(value) -> integer -- return number of occurrences of value", shlist.count.__doc__)
EXPECT_EQ("L.extend(iterable) -> None -- extend list by appending elements from the iterable", shlist.extend.__doc__)
EXPECT_EQ("L.index(value, [start, [stop]]) -> integer -- return first index of value.\nRaises ValueError if the value is not present.", shlist.index.__doc__)
EXPECT_EQ("L.insert(index, object) -> None -- insert object at index", shlist.insert.__doc__)
EXPECT_EQ("L.pop([index]) -> item -- remove and return item at index (default last).\nRaises IndexError if list is empty or index is out of range.", shlist.pop.__doc__)
EXPECT_EQ("L.remove(value) -> None -- remove first occurrence of value", shlist.remove.__doc__)
EXPECT_EQ("L.remove_all() -> None -- remove all elements from the list", shlist.remove_all.__doc__)
EXPECT_EQ("L.reverse() -> None -- reverse *IN PLACE*", shlist.reverse.__doc__)
EXPECT_EQ("L.sort(key=None, reverse=False) -> None -- stable sort *IN PLACE*", shlist.sort.__doc__)

#@<> isinstance
EXPECT_TRUE(isinstance(shlist(), list))

#@<> available members
EXPECT_EQ([
    "__add__",
    "__class__",
    "__contains__",
    "__delattr__",
    "__delitem__",
    "__dir__",
    "__doc__",
    "__eq__",
    "__format__",
    "__ge__",
    "__getattribute__",
    "__getitem__",
    "__getstate__",
    "__gt__",
    "__hash__",
    "__iadd__",
    "__imul__",
    "__init__",
    "__init_subclass__",
    "__iter__",
    "__le__",
    "__len__",
    "__lt__",
    "__mul__",
    "__ne__",
    "__new__",
    "__reduce__",
    "__reduce_ex__",
    "__repr__",
    "__reversed__",
    "__rmul__",
    "__setattr__",
    "__setitem__",
    "__setstate__",
    "__sizeof__",
    "__str__",
    "__subclasshook__",
    "append",
    "clear",
    "copy",
    "count",
    "extend",
    "index",
    "insert",
    "pop",
    "remove",
    "remove_all",
    "reverse",
    "sort"
], dir(shlist()))

#@<> constructors
EXPECT_EQ(list(), shlist())
EXPECT_EQ(list([1, 2, 3]), shlist([1, 2, 3]))
EXPECT_EQ(list(range(3)), shlist(range(3)))

#@<> pickle
SETUP("ignored")
p = pickle.loads(pickle.dumps(actual))
EXPECT_EQ(actual, p)
EXPECT_EQ(actual.__class__, p.__class__)

SETUP("ignored", range(11))
p = pickle.loads(pickle.dumps(actual))
EXPECT_EQ(actual, p)
EXPECT_EQ(actual.__class__, p.__class__)

#@<> __add__
SETUP("__add__")

TEST_LIST_ARG([])
TEST_LIST_ARG([1, 2, 3])
TEST("abc")
TEST(7)

SETUP("__add__", range(11))

TEST_LIST_ARG([])
TEST_LIST_ARG([1, 2, 3])
TEST("abc")
TEST(7)

SETUP("+")

TEST_OP_LIST_ARG([])
TEST_OP_LIST_ARG(range(3))
TEST_OP("abc")
TEST_OP(7)

SETUP("+", range(11))

TEST_OP_LIST_ARG([])
TEST_OP_LIST_ARG(range(3))
TEST_OP("abc")
TEST_OP(7)

## exceptions
# can only concatenate list (not \"%.200s\") to list
TEST("")

#@<> __class__
EXPECT_EQ(shlist, shlist().__class__)

#@<> __contains__
SETUP("__contains__")

TEST(None)
TEST(1)

SETUP("__contains__", range(11))

TEST(None)
TEST(1)

#@<> __delattr__
# __delattr__ comes from the object class
EXPECT_EQ(object.__delattr__, shlist.__delattr__)

#@<> __delitem__
SETUP("__delitem__", range(100))

## single item
for i in range(11):
    TEST(i)
    TEST(-i)

## slices
for s in all_slices:
    SETUP("__delitem__", range(11))
    TEST(s)

#@<> __dir__
all_methods = sorted(shlist().__dir__())
# list does not have __getstate__(), __setstate__() and remove_all()
all_methods.remove("__getstate__")
all_methods.remove("__setstate__")
all_methods.remove("remove_all")

EXPECT_EQ(sorted(list().__dir__()), all_methods)

#@<> __doc__
EXPECT_EQ("""List() -> new empty shcore array

Creates a new instance of a shcore array object.""", shlist.__doc__)

#@<> __eq__, __ge__, __gt__, __le__, __lt__, __ne__
ops = [ "==", ">=", ">", "<=", "<", "!="]

# we cannot call i.e. __eq__ here, because list.__eq__(shlist) will fail
# in comparisons below Python will always use shlist.__eq__():
#   If the operands are of different types, and right operand’s type is a direct
#   or indirect subclass of the left operand’s type, the reflected method of the
#   right operand has priority, otherwise the left operand’s method has priority.

for op in ops:
    SETUP(op)
    TEST_OP_LIST_ARG([])
    TEST_OP_LIST_ARG(range(3))

for op in ops:
    SETUP(op, range(3))
    TEST_OP_LIST_ARG([])
    TEST_OP_LIST_ARG(range(2))
    TEST_OP_LIST_ARG(range(3))
    TEST_OP_LIST_ARG(range(4))
    TEST_OP_LIST_ARG([0, 1, 1])
    TEST_OP_LIST_ARG([0, 1, 3])

#@<> __format__
# __format__ comes from the object class
EXPECT_EQ(object.__format__, shlist.__format__)

SETUP("__format__")

TEST("")

SETUP("__format__", range(11))

TEST("")

#@<> __getattribute__
EXPECT_EQ("L.__getitem__(y) <==> L[y]", shlist().__getattribute__('__getitem__').__doc__)
EXPECT_EQ("L.remove_all() -> None -- remove all elements from the list", shlist().__getattribute__('remove_all').__doc__)

#@<> __getitem__
SETUP("__getitem__", range(11))

## exceptions
# list indices must be integers or slices, not %.200s
TEST("")
# list index out of range
TEST(100)
TEST(-100)

## single item
for i in range(11):
    TEST(i)
    TEST(-i)

## slices
for s in all_slices:
    TEST(s)

#@<> __getstate__
SETUP("__getstate__", range(11))
EXPECT_EQ(magic_state, actual.__getstate__())

#@<> __hash__
SETUP("__hash__", range(11))
# __hash__ is None
EXPECT_EQ(None, actual.__hash__)
EXPECT_EQ(expected.__hash__, actual.__hash__)

#@<> __iadd__
SETUP("__iadd__")

TEST_LIST_ARG([])
TEST_LIST_ARG(range(3))
TEST("abc")
TEST(7)

SETUP("__iadd__", range(11))

TEST_LIST_ARG([])
TEST_LIST_ARG(range(3))
TEST("abc")
TEST(7)

SETUP("+=")

TEST_OP_LIST_ARG([])
TEST_OP_LIST_ARG(range(3))
TEST_OP("abc")
TEST_OP(7)

SETUP("+=", range(11))

TEST_OP_LIST_ARG([])
TEST_OP_LIST_ARG(range(3))
TEST_OP("abc")
TEST_OP(7)

## self assignment
EXPECT_EQ(expected.__iadd__(expected), actual.__iadd__(actual))

#@<> __imul__
SETUP("__imul__")

TEST(-1)
TEST(0)
TEST(1)
TEST(7)

SETUP("__imul__", range(11))
TEST(-1)

SETUP("__imul__", range(11))
TEST(0)

SETUP("__imul__", range(11))

TEST(1)
TEST(7)
TEST("abc")

SETUP("*=")

TEST_OP(-1)
TEST_OP(0)
TEST_OP(1)
TEST_OP(7)

SETUP("*=", range(11))
TEST_OP(-1)

SETUP("*=", range(11))
TEST_OP(0)

SETUP("*=", range(11))

TEST_OP(1)
TEST_OP(7)
TEST_OP("abc")

#@<> __init__ + __new__
s = shlist.__new__(shlist)
s.__init__([1, 2, 3])
EXPECT_EQ([1, 2, 3], s)
EXPECT_EQ(shlist, s.__class__)

#@<> __init_subclass__
# we should be using default method from object, which throws if it's called with any arguments (correct implementation takes one)
EXPECT_THROWS(lambda: shlist.__init_subclass__(1), "TypeError: __init_subclass__() takes no arguments (1 given)")

#@<> __iter__
SETUP("__iter__")

it = actual.__iter__()
EXPECT_EQ(0, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p2 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(0, it1.__length_hint__())
EXPECT_THROWS(lambda: it1.__next__(), "StopIteration")

it2 = pickle.loads(p2)
EXPECT_EQ(0, it2.__length_hint__())
EXPECT_THROWS(lambda: it2.__next__(), "StopIteration")

SETUP("__iter__", range(3))

it = actual.__iter__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ(0, it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ(1, it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ(2, it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ(0, it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ(1, it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ(2, it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> __len__
SETUP("__len__")
TEST()

SETUP("__len__", range(11))
TEST()

#@<> __mul__
SETUP("__mul__")

TEST(-1)
TEST(0)
TEST(1)
TEST(7)

SETUP("__mul__", range(11))
TEST(-1)

SETUP("__mul__", range(11))
TEST(0)

SETUP("__mul__", range(11))

TEST(1)
TEST(7)
TEST("abc")

SETUP("*")

TEST_OP(-1)
TEST_OP(0)
TEST_OP(1)
TEST_OP(7)

SETUP("*", range(11))
TEST_OP(-1)

SETUP("*", range(11))
TEST_OP(0)

SETUP("*", range(11))

TEST_OP(1)
TEST_OP(7)
TEST_OP("abc")

#@<> __reduce__
# __reduce__ comes from the object class
EXPECT_EQ(object.__reduce__, shlist.__reduce__)

#@<> __reduce_ex__
# __reduce_ex__ comes from the object class
EXPECT_EQ(object.__reduce_ex__, shlist.__reduce_ex__)

#@<> __repr__
SETUP("__repr__")
TEST()

SETUP("__repr__", range(11))
TEST()

#@<> __reversed__
SETUP("__reversed__")

it = actual.__reversed__()
EXPECT_EQ(0, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p2 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(0, it1.__length_hint__())
EXPECT_THROWS(lambda: it1.__next__(), "StopIteration")

it2 = pickle.loads(p2)
EXPECT_EQ(0, it2.__length_hint__())
EXPECT_THROWS(lambda: it2.__next__(), "StopIteration")

SETUP("__reversed__", range(3))

it = actual.__reversed__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ(2, it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ(1, it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ(0, it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ(2, it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ(1, it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ(0, it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> __rmul__
SETUP("__rmul__")

TEST(-1)
TEST(0)
TEST(1)
TEST(7)

SETUP("__rmul__", range(11))
TEST(-1)

SETUP("__rmul__", range(11))
TEST(0)

SETUP("__rmul__", range(11))

TEST(1)
TEST(7)
TEST("abc")

SETUP("*")

TEST_OP(-1)
TEST_OP(0)
TEST_OP(1)
TEST_OP(7)

SETUP("*", range(11))
TEST_OP(-1)

SETUP("*", range(11))
TEST_OP(0)

SETUP("*", range(11))

TEST_OP(1)
TEST_OP(7)
TEST_OP("abc")

#@<> __setattr__
# __setattr__ comes from the object class
EXPECT_EQ(object.__setattr__, shlist.__setattr__)

#@<> __setitem__
SETUP("__setitem__", range(11))

## exceptions
# can only assign an iterable
TEST(slice(2, 7), 7)
# must assign iterable to extended slice
TEST(slice(2, 7, 2), 7)
# attempt to assign sequence of size %zd to extended slice of size %zd
TEST(slice(2, 7, 2), [])
# list indices must be integers or slices, not %.200s
TEST("", 7)
# list assignment index out of range
TEST(100, 100)
TEST(-100, 100)

## single item
for i in range(11):
    TEST(i, i * i)
    TEST(-i, i + i)

## self-assignment with slices
for s in slices + [slice(None, None, -1)]:
    EXPECT_EQ(expected.__setitem__(s, expected), actual.__setitem__(s, actual), s)

## slices
length = 11

for s in slices:
    SETUP("__setitem__", range(length))
    TEST_LIST_ARG(random_list(length), s)

for s in step_slices:
    SETUP("__setitem__", range(length))
    # assign list whose length is equal to the number of elements selected by the slice
    TEST_LIST_ARG(random_list(*s.indices(length)), s)

TEST(slice(1, 2), [0, 1])
TEST(slice(1, 1), [0, 1])
TEST(slice(1, 1), [])
TEST(slice(1, 3), [0])
TEST(slice(1, 3), [0, 1])
TEST(slice(1, 3), [0, 1, 2])
TEST(slice(1, 3), [0, 1, 2, 3])
TEST(slice(1, 7, 2), [0, 1, 2])
TEST(slice(1, 7, 2), "abc")

#@<> __setstate__
SETUP("__setstate__", range(11))
EXPECT_THROWS(lambda: actual.__setstate__(None), "TypeError: an integer is required")
EXPECT_THROWS(lambda: actual.__setstate__(magic_state - 1), "RuntimeError: Wrong magic number used")
EXPECT_EQ(None, actual.__setstate__(magic_state))

#@<> __sizeof__
# our implementation always returns the same value
EXPECT_EQ(shlist().__sizeof__(), shlist(range(11)).__sizeof__())
EXPECT_NE(list().__sizeof__(), list(range(11)).__sizeof__())

#@<> __str__
SETUP("__str__")
TEST()

SETUP("__str__", range(11))
TEST()

#@<> __subclasshook__
# __subclasshook__ comes from the object class
EXPECT_EQ(NotImplemented, shlist.__subclasshook__())

#@<> append
SETUP("append")

TEST()
TEST(1)
TEST("a")
TEST_LIST_ARG(range(2))

#@<> clear
SETUP("clear")
TEST()

SETUP("clear", range(11))
TEST()

#@<> copy
SETUP("copy")
TEST()

SETUP("copy", range(11))
TEST()

c = actual.copy()
c[0] = "a"

EXPECT_NE(actual[0], c[0])

#@<> count
SETUP("count")
TEST(0)
TEST(1)
TEST(100)

SETUP("count", range(11))
TEST(0)
TEST(1)
TEST(100)

SETUP("count", 10 * [1])
TEST(0)
TEST(1)
TEST(100)

#@<> extend
SETUP("extend")
TEST(0)
TEST_LIST_ARG(range(11))
TEST("ABC")

SETUP("extend", range(11))
TEST(0)
TEST_LIST_ARG(range(11))
TEST("ABC")

#@<> index
SETUP("index")

TEST(1)
TEST(1, 5)
TEST(1, 5, 9)
TEST(1, 9, 5)
TEST(1, -5)
TEST(1, -5, 9)
TEST(1, 9, -5)
TEST(1, -9, -5)
TEST(1, -5, -9)

SETUP("index", range(11))

TEST(1)
TEST(1, 5)
TEST(1, 5, 9)
TEST(1, 9, 5)
TEST(1, -5)
TEST(1, -5, 9)
TEST(1, 9, -5)
TEST(1, -9, -5)
TEST(1, -5, -9)

TEST(7)
TEST(7, 5)
TEST(7, 5, 9)
TEST(7, 9, 5)
TEST(7, -5)
TEST(7, -5, 9)
TEST(7, 9, -5)
TEST(7, -9, -5)
TEST(7, -5, -9)

TEST("a")
TEST("a", 5)
TEST("a", 5, 9)
TEST("a", 9, 5)
TEST("a", -5)
TEST("a", -5, 9)
TEST("a", 9, -5)
TEST("a", -9, -5)
TEST("a", -5, -9)

#@<> insert
SETUP("insert")

TEST(0, 100)
TEST(-5, 101)
TEST(5, 102)

SETUP("insert", range(11))

TEST(12, 100)
TEST(0, 101)
TEST(-5, 102)
TEST(5, 103)
TEST(-50, 104)
TEST(50, 105)

TEST_LIST_ARG(range(4), 7)

#@<> pop
SETUP("pop")

TEST()
TEST(-1)
TEST(0)
TEST(1)

SETUP("pop", range(11))

TEST()
TEST(-1)
TEST(0)
TEST(1)
TEST(-4)
TEST(4)

TEST(100)
TEST(-100)

#@<> remove
SETUP("remove")

TEST()
TEST(-1)
TEST(0)
TEST(1)

SETUP("remove", range(11))

TEST(-1)
TEST(0)
TEST(1)
TEST(7)
TEST(10)
TEST(100)

#@<> remove_all
SETUP("remove_all")
EXPECT_EQ(None, actual.remove_all())
EXPECT_EQ([], actual)

SETUP("remove_all", range(11))
EXPECT_EQ(None, actual.remove_all())
EXPECT_EQ([], actual)

#@<> reverse
SETUP("reverse")
TEST()

SETUP("reverse", range(11))
TEST()

#@<> sort
SETUP("sort")

TEST()
TEST(reverse=True)
TEST(key=lambda x: len(x))
TEST(key=lambda x: len(x), reverse=True)

SETUP("sort", random_list(100000))
TEST()

SETUP("sort", random_list(100000))
TEST(reverse=True)

# `key` function throws an exception
TEST(key=lambda x: len(x))

## test stable sort
SETUP("sort", ["d", "cc", "bb", "a"])

TEST(key=lambda x: len(x))
TEST(key=lambda x: len(x), reverse=True)

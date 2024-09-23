## imports
import pickle
import random
import sys

## constants
shdict = sys.modules["mysqlsh.__shell_python_support__"].Dict

debug = False

## helpers
def exception(exc):
    return tuple([item.replace('mysqlsh.__shell_python_support__.Dict', 'dict').replace("Dict.", "dict.") if isinstance(item, str) else item for idx, item in enumerate(exc.args)])if exc is not None else exc

def SETUP(m, *args, **kwargs):
    global expected
    global actual
    global method
    expected = dict(*args, **kwargs)
    actual = shdict(*args, **kwargs)
    method = m

def SETUP_VIEW(v, m, *args, **kwargs):
    global expected
    global actual
    global method
    expected = dict(*args, **kwargs).__getattribute__(v)()
    actual = shdict(*args, **kwargs).__getattribute__(v)()
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

def TEST_DICT_ARG(init, *args, **kwargs):
    global expected
    global actual
    e = expected.copy()
    a = actual.copy()
    for d in [ dict, shdict ]:
        expected = e.copy()
        actual = a.copy()
        TEST(*args, d(init), **kwargs)

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
    EXECUTE(lambda: func(f"expected {method} v", g, l), lambda: func(f"actual {method} v", g, l), f"dict {method} {str(v)}", cleanup)
    EXECUTE(lambda: func(f"v {method} expected", g, l), lambda: func(f"v {method} actual", g, l), f"{str(v)} {method} dict", cleanup)
    EXECUTE(lambda: func(f"expected {method} expected"), lambda: func(f"actual {method} actual"), f"dict {method} dict", cleanup)

def TEST_OP_DICT_ARG(*args, **kwargs):
    global expected
    global actual
    e = expected.copy()
    a = actual.copy()
    for d in [ dict, shdict ]:
        expected = e.copy()
        actual = a.copy()
        TEST_OP(d(*args, **kwargs))

def assign_none_to_mapping():
    global actual
    actual.mapping = None

#@<> docs
EXPECT_EQ("D.__contains__(key) -- True if D has a key k, else False.", shdict.__contains__.__doc__)
EXPECT_EQ("D.__dir__() -- returns list of valid attributes", shdict.__dir__.__doc__)
EXPECT_EQ("D.__getitem__(y) <==> D[y]", shdict.__getitem__.__doc__)
EXPECT_EQ("D.__reversed__() -- return a reverse iterator over the dict keys.", shdict.__reversed__.__doc__)
EXPECT_EQ("D.__sizeof__() -> size of D in memory, in bytes", shdict.__sizeof__.__doc__)
EXPECT_EQ("D.clear() -> None.  Remove all items from D.", shdict.clear.__doc__)
EXPECT_EQ("D.copy() -> a shallow copy of D", shdict.copy.__doc__)
EXPECT_EQ("fromkeys($type, iterable, value=None) -- Returns a new dict with keys from iterable and values equal to value.", shdict.fromkeys.__doc__)
EXPECT_EQ("D.get(k[,d]) -> D[k] if k in D, else d.  d defaults to None.", shdict.get.__doc__)
EXPECT_EQ("D.has_key(key) -- True if D has a key k, else False.", shdict.has_key.__doc__)
EXPECT_EQ("D.items() -> a set-like object providing a view on D's items", shdict.items.__doc__)
EXPECT_EQ("D.keys() -> a set-like object providing a view on D's keys", shdict.keys.__doc__)
EXPECT_EQ("D.pop(k[,d]) -> v, remove specified key and return the corresponding value.\nIf key is not found, d is returned if given, otherwise KeyError is raised", shdict.pop.__doc__)
EXPECT_EQ("D.popitem() -> (k, v), remove and return some (key, value) pair as a\n2-tuple; but raise KeyError if D is empty.", shdict.popitem.__doc__)
EXPECT_EQ("D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if k not in D", shdict.setdefault.__doc__)
EXPECT_EQ("D.update([E, ]**F) -> None.  Update D from dict/iterable E and F.\nIf E is present and has a .keys() method, then does:  for k in E: D[k] = E[k]\nIf E is present and lacks a .keys() method, then does:  for k, v in E: D[k] = v\nIn either case, this is followed by: for k in F:  D[k] = F[k]", shdict.update.__doc__)
EXPECT_EQ("D.values() -> an object providing a view on D's values", shdict.values.__doc__)

#@<> isinstance
EXPECT_TRUE(isinstance(shdict(), dict))

#@<> available members
dict_members = [
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
    "__gt__",
    "__hash__",
    "__init__",
    "__init_subclass__",
    "__ior__",
    "__iter__",
    "__le__",
    "__len__",
    "__lt__",
    "__ne__",
    "__new__",
    "__or__",
    "__reduce__",
    "__reduce_ex__",
    "__repr__",
    "__reversed__",
    "__ror__",
    "__setattr__",
    "__setitem__",
    "__sizeof__",
    "__str__",
    "__subclasshook__",
    "clear",
    "copy",
    "fromkeys",
    "get",
    "has_key",
    "items",
    "keys",
    "pop",
    "popitem",
    "setdefault",
    "update",
    "values"
]

if sys.hexversion >= 0x03090000:
    dict_members.append("__class_getitem__")

if sys.hexversion >= 0x030b0000:
    dict_members.append("__getstate__")

EXPECT_EQ(sorted(dict_members), dir(shdict()))

#@<> constructors
EXPECT_EQ(dict(), shdict())
EXPECT_EQ(dict(one=1, two=2, three=3), shdict(one=1, two=2, three=3))
EXPECT_EQ(dict(zip(['one', 'two', 'three'], [1, 2, 3])), shdict(zip(['one', 'two', 'three'], [1, 2, 3])))
EXPECT_EQ(dict([('two', 2), ('one', 1), ('three', 3)]), shdict([('two', 2), ('one', 1), ('three', 3)]))
EXPECT_EQ(dict({'three': 3, 'one': 1, 'two': 2}), shdict({'three': 3, 'one': 1, 'two': 2}))
EXPECT_EQ(dict({'one': 1, 'three': 3}, two=2), shdict({'one': 1, 'three': 3}, two=2))

#@<> pickle
SETUP("ignored")
for proto in range(2, pickle.HIGHEST_PROTOCOL + 1):
    p = pickle.loads(pickle.dumps(actual, protocol = proto))
    EXPECT_EQ(actual, p)
    EXPECT_EQ(actual.__class__, p.__class__)

SETUP("ignored", {'one': 1, 'two': 2, 'three': 3})
for proto in range(2, pickle.HIGHEST_PROTOCOL + 1):
    p = pickle.loads(pickle.dumps(actual, protocol = proto))
    EXPECT_EQ(actual, p)
    EXPECT_EQ(actual.__class__, p.__class__)

#@<> __class__
EXPECT_EQ(shdict, shdict().__class__)

#@<> __contains__
SETUP("__contains__")

TEST(None)
TEST(1)
TEST('one')

SETUP("__contains__", {'one': 1, 'two': 2, 'three': 3})

TEST(None)
TEST(1)
TEST('one')

#@<> __delattr__
# __delattr__ comes from the object class
EXPECT_EQ(object.__delattr__, shdict.__delattr__)

#@<> __delitem__
SETUP("__delitem__", {'one': 1, 'two': 2, 'three': 3})

TEST(None)
TEST(1)
TEST('one')
TEST('four')

#@<> __dir__
all_methods = sorted(shdict().__dir__())
# dict does not have has_key()
all_methods.remove("has_key")

if sys.hexversion < 0x03090000:
    all_methods.remove("__ior__")
    all_methods.remove("__or__")
    all_methods.remove("__ror__")

EXPECT_EQ(sorted(dict().__dir__()), all_methods)

#@<> __doc__
EXPECT_EQ("""Dict() -> new empty shcore dictionary

Creates a new instance of a shcore dictionary object.""", shdict.__doc__)

#@<> __eq__, __ge__, __gt__, __le__, __lt__, __ne__
ops = [ "==", ">=", ">", "<=", "<", "!="]

# we cannot call i.e. __eq__ here, because dict.__eq__(shdict) will fail
# in comparisons below Python will always use shdict.__eq__():
#   If the operands are of different types, and right operand’s type is a direct
#   or indirect subclass of the left operand’s type, the reflected method of the
#   right operand has priority, otherwise the left operand’s method has priority.

for op in ops:
    SETUP(op)
    TEST_OP_DICT_ARG({})
    TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

for op in ops:
    SETUP(op, {'one': 1, 'two': 2, 'three': 3})
    TEST_OP_DICT_ARG({})
    TEST_OP_DICT_ARG({'one': 1, 'two': 2})
    TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})
    TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 4})
    TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'four': 3})
    TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3, 'four': 4})

#@<> __format__
# __format__ comes from the object class
EXPECT_EQ(object.__format__, shdict.__format__)

# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP("__format__")

EXPECT_EQ("{}", actual.__format__(""))

SETUP("__format__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ('{"one": 1, "three": 3, "two": 2}', actual.__format__(""))

#@<> __getattribute__
EXPECT_EQ("D.__getitem__(y) <==> D[y]", shdict().__getattribute__('__getitem__').__doc__)
EXPECT_EQ("D.has_key(key) -- True if D has a key k, else False.", shdict().__getattribute__('has_key').__doc__)

#@<> __getitem__
SETUP("__getitem__", {'one': 1, 'two': 2, 'three': 3})

TEST(None)
TEST(1)
TEST('one')
TEST('two')
TEST('three')

#@<> __hash__
SETUP("__hash__", {'one': 1, 'two': 2, 'three': 3})
# __hash__ is None
EXPECT_EQ(None, actual.__hash__)
EXPECT_EQ(expected.__hash__, actual.__hash__)

#@<> __init__ + __new__
s = shdict.__new__(shdict)
s.__init__({'one': 1, 'two': 2, 'three': 3})
EXPECT_EQ({'one': 1, 'two': 2, 'three': 3}, s)
EXPECT_EQ(shdict, s.__class__)

#@<> __init_subclass__
# we should be using default method from object, which throws if it's called with any arguments (correct implementation takes one)
EXPECT_THROWS(lambda: shdict.__init_subclass__(1), "__init_subclass__() takes no arguments (1 given)")

#@<> __ior__ {sys.hexversion >= 0x03090000}
SETUP("__ior__")

TEST(-1)
TEST(None)
TEST((('a'), ('b')))
TEST({'one': 1})
TEST({'one': 2})
TEST((('a', 1), ('b', 2)))

TEST_DICT_ARG({})
TEST_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

SETUP("__ior__", {'one': 3, 'two': 2, 'three': 1})

TEST(-1)
TEST(None)
TEST((('a'), ('b')))
TEST({'one': 1})
TEST({'one': 2})
TEST((('a', 1), ('b', 2)))

TEST_DICT_ARG({})
TEST_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

SETUP("|=")

TEST_OP(-1)
TEST_OP(None)
TEST_OP((('a'), ('b')))
TEST_OP({'one': 1})
TEST_OP({'one': 2})
TEST_OP((('a', 1), ('b', 2)))

TEST_OP_DICT_ARG({})
TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

SETUP("|=", {'one': 3, 'two': 2, 'three': 1})

TEST_OP(-1)
TEST_OP(None)
TEST_OP((('a'), ('b')))
TEST_OP({'one': 1})
TEST_OP({'one': 2})
TEST_OP((('a', 1), ('b', 2)))

TEST_OP_DICT_ARG({})
TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

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

SETUP("__iter__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__iter__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ('a', it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ('b', it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ('c', it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ('a', it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ('b', it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ('c', it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> __len__
SETUP("__len__")
TEST()

SETUP("__len__", {'one': 1, 'two': 2, 'three': 3})
TEST()

#@<> __or__ {sys.hexversion >= 0x03090000}
SETUP("__or__")

TEST(-1)
TEST(None)
TEST((('a'), ('b')))
TEST({})
TEST({'one': 1})
TEST((('a', 1), ('b', 2)))

SETUP("__or__", {'one': 3, 'two': 2, 'three': 1})

TEST(-1)
TEST(None)
TEST((('a'), ('b')))
TEST({})
TEST({'one': 1})
TEST((('a', 1), ('b', 2)))

SETUP("|")

TEST_OP(-1)
TEST_OP(None)
TEST_OP((('a'), ('b')))
TEST_OP({})
TEST_OP({'one': 1})
TEST_OP((('a', 1), ('b', 2)))

TEST_OP_DICT_ARG({})
TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

SETUP("|", {'one': 3, 'two': 2, 'three': 1})

TEST_OP(-1)
TEST_OP(None)
TEST_OP((('a'), ('b')))
TEST_OP({})
TEST_OP({'one': 1})
TEST_OP((('a', 1), ('b', 2)))

TEST_OP_DICT_ARG({})
TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

#@<> __reduce__
# __reduce__ comes from the object class
EXPECT_EQ(object.__reduce__, shdict.__reduce__)

#@<> __reduce_ex__
# __reduce_ex__ comes from the object class
EXPECT_EQ(object.__reduce_ex__, shdict.__reduce_ex__)

#@<> __repr__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP("__repr__")

EXPECT_EQ("{}", actual.__repr__())

SETUP("__repr__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ('{"one": 1, "three": 3, "two": 2}', actual.__repr__())

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

SETUP("__reversed__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__reversed__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ('c', it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ('b', it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ('a', it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ('c', it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ('b', it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ('a', it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> __ror__ {sys.hexversion >= 0x03090000}
SETUP("__ror__")

TEST(-1)
TEST(None)
TEST((('a'), ('b')))
TEST({})
TEST({'one': 1})
TEST((('a', 1), ('b', 2)))

SETUP("__ror__", {'one': 3, 'two': 2, 'three': 1})

TEST(-1)
TEST(None)
TEST((('a'), ('b')))
TEST({})
TEST({'one': 1})
TEST((('a', 1), ('b', 2)))

SETUP("|")

TEST_OP(-1)
TEST_OP(None)
TEST_OP((('a'), ('b')))
TEST_OP({})
TEST_OP({'one': 1})
TEST_OP((('a', 1), ('b', 2)))

TEST_OP_DICT_ARG({})
TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

SETUP("|", {'one': 3, 'two': 2, 'three': 1})

TEST_OP(-1)
TEST_OP(None)
TEST_OP((('a'), ('b')))
TEST_OP({})
TEST_OP({'one': 1})
TEST_OP((('a', 1), ('b', 2)))

TEST_OP_DICT_ARG({})
TEST_OP_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

#@<> __setattr__
# __setattr__ comes from the object class
EXPECT_EQ(object.__setattr__, shdict.__setattr__)

#@<> __setitem__
SETUP("__setitem__")

TEST("one", 1)
TEST("one", 2)
TEST("two", 2)
TEST("three", 4)
TEST("four", None)

# we do not allow for non-string keys, dict() works just fine
EXPECT_NO_THROWS(lambda: expected.__setitem__(4, "four"))
EXPECT_THROWS(lambda: actual.__setitem__(4, "four"), "KeyError: 'shell.Dict key must be a string'")
# trying to get a non-string key results in KeyError
EXPECT_EQ("four", expected.__getitem__(4))
EXPECT_THROWS(lambda: actual.__getitem__(4), "KeyError: 4")

SETUP("__setitem__", {'one': 1, 'two': 2, 'three': 3})

TEST("one", 1)
TEST("one", 2)
TEST("two", 2)
TEST("three", 3)
TEST("four", None)

#@<> __sizeof__
# our implementation always returns the same value
EXPECT_EQ(shdict().__sizeof__(), shdict({'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5, 'six': 6, 'seven': 7, 'eight': 8, 'nine': 9, 'ten': 10}).__sizeof__())
EXPECT_NE(dict().__sizeof__(), dict({'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5, 'six': 6, 'seven': 7, 'eight': 8, 'nine': 9, 'ten': 10}).__sizeof__())

#@<> __str__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP("__str__")

EXPECT_EQ("{}", actual.__str__())

SETUP("__str__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ('{"one": 1, "three": 3, "two": 2}', actual.__str__())

#@<> __subclasshook__
# __subclasshook__ comes from the object class
EXPECT_EQ(NotImplemented, shdict.__subclasshook__(None))

#@<> clear
SETUP("clear")
TEST()

SETUP("clear", {'one': 1, 'two': 2, 'three': 3})
TEST()

#@<> copy
SETUP("copy")
TEST()

SETUP("copy", {'one': 1, 'two': 2, 'three': 3})
TEST()

c = actual.copy()
c['one'] = "a"

EXPECT_NE(actual['one'], c['one'])

c = expected.copy()
c['one'] = "a"

EXPECT_NE(expected['one'], c['one'])

#@<> fromkeys
SETUP("fromkeys")

TEST(['a', 'b', 'c'])
TEST(['a', 'b', 'c'], 'xyz')

# keys are converted to strings, dict() doesn't do that
EXPECT_EQ({"1": "abc", "2": "abc", "3": "abc"}, shdict.fromkeys([1, 2, 3], 'abc'))
EXPECT_EQ({1: 'abc', 2: 'abc', 3: 'abc'}, dict.fromkeys([1, 2, 3], 'abc'))

#@<> get
SETUP("get")

TEST(0)
TEST(0, 'one')
TEST('one')
TEST('one', 2)

SETUP("get", {'one': 1, 'two': 2, 'three': 3})

TEST(0)
TEST(0, 'one')
TEST('one')
TEST('one', 2)

#@<> has_key
SETUP("has_key")

EXPECT_EQ(False, actual.has_key(None))
EXPECT_EQ(False, actual.has_key(1))
EXPECT_EQ(False, actual.has_key('one'))

SETUP("has_key", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ(False, actual.has_key(None))
EXPECT_EQ(False, actual.has_key(1))
EXPECT_EQ(True, actual.has_key('one'))

#@<> items
SETUP("items")
TEST()

SETUP("items", {'one': 1, 'two': 2, 'three': 3})
TEST()

expected_view = expected.items()
actual_view = actual.items()

EXPECT_EQ(len(expected_view), len(actual_view))
expected['four'] = 4
actual['four'] = 4
EXPECT_EQ(len(expected_view), len(actual_view))

#@<> items - members
items_members = [
    "__and__",
    "__class__",
    "__contains__",
    "__delattr__",
    "__dir__",
    "__doc__",
    "__eq__",
    "__format__",
    "__ge__",
    "__getattribute__",
    "__getitem__",
    "__gt__",
    "__hash__",
    "__init__",
    "__init_subclass__",
    "__iter__",
    "__le__",
    "__len__",
    "__lt__",
    "__ne__",
    "__new__",
    "__or__",
    "__rand__",
    "__reduce__",
    "__reduce_ex__",
    "__repr__",
    "__reversed__",
    "__ror__",
    "__rsub__",
    "__rxor__",
    "__setattr__",
    "__sizeof__",
    "__str__",
    "__sub__",
    "__subclasshook__",
    "__xor__",
    "isdisjoint",
    "mapping"
]

if sys.hexversion >= 0x030b0000:
    items_members.append("__getstate__")

EXPECT_EQ(sorted(items_members), dir(shdict().items()))

#@<> items - __and__
SETUP_VIEW("items", "__and__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__and__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "&")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "&", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __class__
SETUP_VIEW("items", "__class__")

EXPECT_EQ("dict_items", expected.__class__.__name__)
EXPECT_EQ("Dict_items", actual.__class__.__name__)

#@<> items - __contains__
SETUP_VIEW("items", "__contains__")

TEST(-1)
TEST(None)
TEST(set())
TEST(('one', 1))
TEST(('one', 2))
TEST(('four', 4))

SETUP_VIEW("items", "__contains__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(('one', 1))
TEST(('one', 2))
TEST(('four', 4))

#@<> items - __delattr__
SETUP_VIEW("items", "__delattr__")
# __delattr__ comes from the object class
EXPECT_EQ(object.__delattr__, actual.__class__.__delattr__)

#@<> items - __dir__
SETUP_VIEW("items", "__dir__")
all_methods = sorted(actual.__dir__())
# dict_items does not have __getitem__()
all_methods.remove("__getitem__")

if sys.hexversion < 0x030A0000:
    all_methods.remove("mapping")

EXPECT_EQ(sorted(expected.__dir__()), all_methods)

#@<> items - __doc__
SETUP_VIEW("items", "__doc__")
# __doc__ is None
EXPECT_EQ(None, actual.__doc__)
EXPECT_EQ(expected.__doc__, actual.__doc__)

#@<> items - __eq__, __ge__, __gt__, __le__, __lt__, __ne__
ops = [ "==", ">=", ">", "<=", "<", "!="]

for op in ops:
    SETUP_VIEW("items", op)
    TEST_OP(set())
    TEST_OP(set([('one', 1)]))

for op in ops:
    SETUP_VIEW("items", op, {'one': 1, 'two': 2, 'three': 3})
    TEST_OP(set())
    TEST_OP(set([('one', 1)]))
    TEST_OP(set([('one', 1), ('two', 2)]))
    TEST_OP(set([('one', 1), ('two', 2), ('three', 3)]))
    TEST_OP(set([('one', 1), ('two', 2), ('three', 3), ('four', 4)]))

#@<> items - __format__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("items", "__format__")

EXPECT_EQ("Dict_items([])", actual.__format__(""))

SETUP_VIEW("items", "__format__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_items([('one', 1), ('three', 3), ('two', 2)])", actual.__format__(""))

# __format__ comes from the object class
EXPECT_EQ(object.__format__, actual.__class__.__format__)

#@<> items - __getattribute__
SETUP_VIEW("items", "__getattribute__")
EXPECT_EQ("__format__", shdict().__getattribute__('__format__').__name__)

#@<> items - __getitem__
SETUP_VIEW("items", "__getitem__")

EXPECT_THROWS(lambda: actual.__getitem__("a"), "TypeError: list indices must be integers or slices, not str")

EXPECT_THROWS(lambda: actual.__getitem__(-1), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(0), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(1), "IndexError: list index out of range")

SETUP_VIEW("items", "__getitem__", {'a': 1, 'b': 2, 'c': 3})

EXPECT_THROWS(lambda: actual.__getitem__(-4), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(3), "IndexError: list index out of range")

EXPECT_EQ(('a', 1), actual.__getitem__(-3))
EXPECT_EQ(('a', 1), actual.__getitem__(0))

EXPECT_EQ(('b', 2), actual.__getitem__(-2))
EXPECT_EQ(('b', 2), actual.__getitem__(1))

EXPECT_EQ(('c', 3), actual.__getitem__(-1))
EXPECT_EQ(('c', 3), actual.__getitem__(2))

#@<> items - __hash__
SETUP_VIEW("items", "__hash__")
# __hash__ is None
EXPECT_EQ(None, actual.__hash__)
EXPECT_EQ(expected.__hash__, actual.__hash__)

#@<> items - __init__
SETUP_VIEW("items", "__init__")
# __init__ comes from the object class
EXPECT_EQ(object.__init__, actual.__class__.__init__)

#@<> items - __init_subclass__
SETUP_VIEW("items", "__init_subclass__")
# we should be using default method from object, which throws if it's called with any arguments (correct implementation takes one)
EXPECT_THROWS(lambda: actual.__init_subclass__(1), "__init_subclass__() takes no arguments (1 given)")

#@<> items - __iter__
SETUP_VIEW("items", "__iter__")

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

SETUP_VIEW("items", "__iter__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__iter__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ(('a', 1), it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ(('b', 2), it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ(('c', 3), it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ(('a', 1), it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ(('b', 2), it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ(('c', 3), it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> items - __len__
SETUP_VIEW("items", "__len__")
TEST()

SETUP_VIEW("items", "__len__", {'one': 1, 'two': 2, 'three': 3})
TEST()

#@<> items - __new__
SETUP_VIEW("items", "__new__")
# __new__ comes from the object class
EXPECT_EQ(object.__new__, actual.__class__.__new__)

#@<> items - __or__
SETUP_VIEW("items", "__or__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__or__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "|")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "|", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __rand__
SETUP_VIEW("items", "__rand__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__rand__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "&")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "&", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __reduce__
SETUP_VIEW("items", "__reduce__")
# __reduce__ comes from the object class
EXPECT_EQ(object.__reduce__, actual.__class__.__reduce__)

#@<> items - __reduce_ex__
SETUP_VIEW("items", "__reduce_ex__")
# __reduce_ex__ comes from the object class
EXPECT_EQ(object.__reduce_ex__, actual.__class__.__reduce_ex__)

#@<> items - __repr__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("items", "__repr__")

EXPECT_EQ("Dict_items([])", actual.__repr__())

SETUP_VIEW("items", "__repr__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_items([('one', 1), ('three', 3), ('two', 2)])", actual.__repr__())

#@<> items - __reversed__
SETUP_VIEW("items", "__reversed__")

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

SETUP_VIEW("items", "__reversed__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__reversed__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ(('c', 3), it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ(('b', 2), it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ(('a', 1), it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ(('c', 3), it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ(('b', 2), it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ(('a', 1), it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> items - __ror__
SETUP_VIEW("items", "__ror__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__ror__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "|")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "|", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __rsub__
SETUP_VIEW("items", "__rsub__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__rsub__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "-")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "-", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __rxor__
SETUP_VIEW("items", "__rxor__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__rxor__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "^")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "^", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __setattr__
SETUP_VIEW("items", "__setattr__")
# __setattr__ comes from the object class
EXPECT_EQ(object.__setattr__, actual.__class__.__setattr__)

#@<> items - __sizeof__
SETUP_VIEW("items", "__sizeof__")
# __sizeof__ comes from the object class
EXPECT_EQ(object.__sizeof__, actual.__class__.__sizeof__)

#@<> items - __str__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("items", "__str__")

EXPECT_EQ("Dict_items([])", actual.__str__())

SETUP_VIEW("items", "__str__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_items([('one', 1), ('three', 3), ('two', 2)])", actual.__str__())

#@<> items - __sub__
SETUP_VIEW("items", "__sub__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__sub__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "-")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "-", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - __subclasshook__
SETUP_VIEW("items", "__subclasshook__")
# __subclasshook__ comes from the object class
EXPECT_EQ(NotImplemented, actual.__class__.__subclasshook__(None))

#@<> items - __xor__
SETUP_VIEW("items", "__xor__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "__xor__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set([('one', 1)]))
TEST(set([('one', 2)]))
TEST(set([('four', 4)]))

SETUP_VIEW("items", "^")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

SETUP_VIEW("items", "^", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set([('one', 1)]))
TEST_OP(set([('one', 2)]))
TEST_OP(set([('four', 4)]))

#@<> items - isdisjoint
SETUP_VIEW("items", "isdisjoint")

TEST(set())
TEST(set([('one', 1)]))

SETUP_VIEW("items", "isdisjoint", {'one': 1, 'two': 2, 'three': 3})

TEST(set())
TEST(set([('one', 1)]))
TEST(set([('four', 4)]))

#@<> items - mapping {sys.hexversion >= 0x030A0000}
SETUP_VIEW("items", "mapping")

EXPECT_EQ(expected.mapping, actual.mapping)

SETUP_VIEW("items", "mapping", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ(expected.mapping, actual.mapping)

#@<> items - mapping - just shdict
SETUP_VIEW("items", "mapping")

EXPECT_EQ("{}", str(actual.mapping))

SETUP_VIEW("items", "mapping", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ('{"one": 1, "three": 3, "two": 2}', str(actual.mapping))

EXPECT_THROWS(assign_none_to_mapping, "AttributeError: attribute 'mapping' of 'Dict_items' objects is not writable")

#@<> keys
SETUP("keys")
TEST()

SETUP("keys", {'one': 1, 'two': 2, 'three': 3})
TEST()

expected_view = expected.keys()
actual_view = actual.keys()

EXPECT_EQ(len(expected_view), len(actual_view))
expected['four'] = 4
actual['four'] = 4
EXPECT_EQ(len(expected_view), len(actual_view))

#@<> keys - members
keys_members = [
    "__and__",
    "__class__",
    "__contains__",
    "__delattr__",
    "__dir__",
    "__doc__",
    "__eq__",
    "__format__",
    "__ge__",
    "__getattribute__",
    "__getitem__",
    "__gt__",
    "__hash__",
    "__init__",
    "__init_subclass__",
    "__iter__",
    "__le__",
    "__len__",
    "__lt__",
    "__ne__",
    "__new__",
    "__or__",
    "__rand__",
    "__reduce__",
    "__reduce_ex__",
    "__repr__",
    "__reversed__",
    "__ror__",
    "__rsub__",
    "__rxor__",
    "__setattr__",
    "__sizeof__",
    "__str__",
    "__sub__",
    "__subclasshook__",
    "__xor__",
    "isdisjoint",
    "mapping"
]

if sys.hexversion >= 0x030b0000:
    keys_members.append("__getstate__")

EXPECT_EQ(sorted(keys_members), dir(shdict().keys()))

#@<> keys - __and__
SETUP_VIEW("keys", "__and__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__and__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "&")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "&", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __class__
SETUP_VIEW("keys", "__class__")

EXPECT_EQ("dict_keys", expected.__class__.__name__)
EXPECT_EQ("Dict_keys", actual.__class__.__name__)

#@<> keys - __contains__
SETUP_VIEW("keys", "__contains__")

TEST(-1)
TEST(None)
TEST('one')
TEST('four')

SETUP_VIEW("keys", "__contains__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST('one')
TEST('four')

#@<> keys - __delattr__
SETUP_VIEW("keys", "__delattr__")
# __delattr__ comes from the object class
EXPECT_EQ(object.__delattr__, actual.__class__.__delattr__)

#@<> keys - __dir__
SETUP_VIEW("keys", "__dir__")
all_methods = sorted(actual.__dir__())
# dict_keys does not have __getitem__()
all_methods.remove("__getitem__")

if sys.hexversion < 0x030A0000:
    all_methods.remove("mapping")

EXPECT_EQ(sorted(expected.__dir__()), all_methods)

#@<> keys - __doc__
SETUP_VIEW("keys", "__doc__")
# __doc__ is None
EXPECT_EQ(None, actual.__doc__)
EXPECT_EQ(expected.__doc__, actual.__doc__)

#@<> keys - __eq__, __ge__, __gt__, __le__, __lt__, __ne__
ops = [ "==", ">=", ">", "<=", "<", "!="]

for op in ops:
    SETUP_VIEW("keys", op)
    TEST_OP(set())
    TEST_OP(set(['one']))

for op in ops:
    SETUP_VIEW("keys", op, {'one': 1, 'two': 2, 'three': 3})
    TEST_OP(set())
    TEST_OP(set(['one']))
    TEST_OP(set(['one', 'two']))
    TEST_OP(set(['one', 'two', 'three']))
    TEST_OP(set(['one', 'two', 'three', 'four']))

#@<> keys - __format__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("keys", "__format__")

EXPECT_EQ("Dict_keys([])", actual.__format__(""))

SETUP_VIEW("keys", "__format__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_keys(['one', 'three', 'two'])", actual.__format__(""))

# __format__ comes from the object class
EXPECT_EQ(object.__format__, actual.__class__.__format__)

#@<> keys - __getattribute__
SETUP_VIEW("keys", "__getattribute__")
EXPECT_EQ("__format__", shdict().__getattribute__('__format__').__name__)

#@<> keys - __getitem__
SETUP_VIEW("keys", "__getitem__")

EXPECT_THROWS(lambda: actual.__getitem__("a"), "TypeError: list indices must be integers or slices, not str")

EXPECT_THROWS(lambda: actual.__getitem__(-1), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(0), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(1), "IndexError: list index out of range")

SETUP_VIEW("keys", "__getitem__", {'a': 1, 'b': 2, 'c': 3})

EXPECT_THROWS(lambda: actual.__getitem__(-4), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(3), "IndexError: list index out of range")

EXPECT_EQ('a', actual.__getitem__(-3))
EXPECT_EQ('a', actual.__getitem__(0))

EXPECT_EQ('b', actual.__getitem__(-2))
EXPECT_EQ('b', actual.__getitem__(1))

EXPECT_EQ('c', actual.__getitem__(-1))
EXPECT_EQ('c', actual.__getitem__(2))

#@<> keys - __hash__
SETUP_VIEW("keys", "__hash__")
# __hash__ is None
EXPECT_EQ(None, actual.__hash__)
EXPECT_EQ(expected.__hash__, actual.__hash__)

#@<> keys - __init__
SETUP_VIEW("keys", "__init__")
# __init__ comes from the object class
EXPECT_EQ(object.__init__, actual.__class__.__init__)

#@<> keys - __init_subclass__
SETUP_VIEW("keys", "__init_subclass__")
# we should be using default method from object, which throws if it's called with any arguments (correct implementation takes one)
EXPECT_THROWS(lambda: actual.__init_subclass__(1), "__init_subclass__() takes no arguments (1 given)")

#@<> keys - __iter__
SETUP_VIEW("keys", "__iter__")

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

SETUP_VIEW("keys", "__iter__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__iter__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ('a', it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ('b', it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ('c', it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ('a', it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ('b', it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ('c', it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> keys - __len__
SETUP_VIEW("keys", "__len__")
TEST()

SETUP_VIEW("keys", "__len__", {'one': 1, 'two': 2, 'three': 3})
TEST()

#@<> keys - __new__
SETUP_VIEW("keys", "__new__")
# __new__ comes from the object class
EXPECT_EQ(object.__new__, actual.__class__.__new__)

#@<> keys - __or__
SETUP_VIEW("keys", "__or__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__or__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "|")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "|", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __rand__
SETUP_VIEW("keys", "__rand__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__rand__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "&")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "&", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __reduce__
SETUP_VIEW("keys", "__reduce__")
# __reduce__ comes from the object class
EXPECT_EQ(object.__reduce__, actual.__class__.__reduce__)

#@<> keys - __reduce_ex__
SETUP_VIEW("keys", "__reduce_ex__")
# __reduce_ex__ comes from the object class
EXPECT_EQ(object.__reduce_ex__, actual.__class__.__reduce_ex__)

#@<> keys - __repr__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("keys", "__repr__")

EXPECT_EQ("Dict_keys([])", actual.__repr__())

SETUP_VIEW("keys", "__repr__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_keys(['one', 'three', 'two'])", actual.__repr__())

#@<> keys - __reversed__
SETUP_VIEW("keys", "__reversed__")

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

SETUP_VIEW("keys", "__reversed__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__reversed__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ('c', it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ('b', it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ('a', it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ('c', it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ('b', it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ('a', it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> keys - __ror__
SETUP_VIEW("keys", "__ror__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__ror__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "|")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "|", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __rsub__
SETUP_VIEW("keys", "__rsub__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__rsub__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "-")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "-", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __rxor__
SETUP_VIEW("keys", "__rxor__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__rxor__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "^")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "^", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __setattr__
SETUP_VIEW("keys", "__setattr__")
# __setattr__ comes from the object class
EXPECT_EQ(object.__setattr__, actual.__class__.__setattr__)

#@<> keys - __sizeof__
SETUP_VIEW("keys", "__sizeof__")
# __sizeof__ comes from the object class
EXPECT_EQ(object.__sizeof__, actual.__class__.__sizeof__)

#@<> keys - __str__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("keys", "__str__")

EXPECT_EQ("Dict_keys([])", actual.__str__())

SETUP_VIEW("keys", "__str__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_keys(['one', 'three', 'two'])", actual.__str__())

#@<> keys - __sub__
SETUP_VIEW("keys", "__sub__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__sub__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "-")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "-", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - __subclasshook__
SETUP_VIEW("keys", "__subclasshook__")
# __subclasshook__ comes from the object class
EXPECT_EQ(NotImplemented, actual.__class__.__subclasshook__(None))

#@<> keys - __xor__
SETUP_VIEW("keys", "__xor__")

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "__xor__", {'one': 1, 'two': 2, 'three': 3})

TEST(-1)
TEST(None)
TEST(set())
TEST(set(['one']))
TEST(set(['four']))

SETUP_VIEW("keys", "^")

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

SETUP_VIEW("keys", "^", {'one': 1, 'two': 2, 'three': 3})

TEST_OP(-1)
TEST_OP(None)
TEST_OP(set())
TEST_OP(set(['one']))
TEST_OP(set(['four']))

#@<> keys - isdisjoint
SETUP_VIEW("keys", "isdisjoint")

TEST(set())
TEST(set(['one']))

SETUP_VIEW("keys", "isdisjoint", {'one': 1, 'two': 2, 'three': 3})

TEST(set())
TEST(set(['one']))
TEST(set(['four']))

#@<> keys - mapping {sys.hexversion >= 0x030A0000}
SETUP_VIEW("keys", "mapping")

EXPECT_EQ(expected.mapping, actual.mapping)

SETUP_VIEW("keys", "mapping", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ(expected.mapping, actual.mapping)

#@<> keys - mapping - just shdict
SETUP_VIEW("keys", "mapping")

EXPECT_EQ("{}", str(actual.mapping))

SETUP_VIEW("keys", "mapping", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ('{"one": 1, "three": 3, "two": 2}', str(actual.mapping))

EXPECT_THROWS(assign_none_to_mapping, "AttributeError: attribute 'mapping' of 'Dict_keys' objects is not writable")

#@<> pop
SETUP("pop")

TEST(0)
TEST(0, 'one')
TEST('one')
TEST('one', 2)

SETUP("pop", {'one': 1, 'two': 2, 'three': 3})

TEST(0)
TEST(0, 'one')
# pop the same key twice, so the second attempt throws
TEST('one')
TEST('one')
TEST('one', 2)

#@<> popitem
SETUP("popitem")

TEST()

SETUP("popitem", {'a': 1, 'b': 2, 'c': 3})

for i in range(len(actual) + 1):
    TEST()

#@<> setdefault
SETUP("setdefault")

TEST("four")
TEST("four", 44)
TEST("four")

TEST("five", 5)
TEST("five")

# we do not allow for non-string keys, dict() works just fine
EXPECT_NO_THROWS(lambda: expected.setdefault(4, "four"))
EXPECT_THROWS(lambda: actual.setdefault(4, "four"), "KeyError: 'shell.Dict key must be a string'")

SETUP("setdefault", {'one': 1, 'two': 2, 'three': 3})

TEST("one")
TEST("one", 2)

TEST("four")
TEST("four", 44)
TEST("four")

TEST("five", 5)
TEST("five")

#@<> update
SETUP("update")
TEST()

SETUP("update")
TEST(one=1, two=2, three=3)

SETUP("update")
# using list here so zip result can be reused
TEST(list(zip(['one', 'two', 'three'], [1, 2, 3])))

SETUP("update")
TEST([('two', 2), ('one', 1), ('three', 3)])

SETUP("update")
TEST({'three': 3, 'one': 1, 'two': 2})

SETUP("update")
TEST({'one': 1, 'three': 3}, two=2)

SETUP("update")
TEST({'one': 1, 'three': 3}, three=2)

SETUP("update")
TEST_DICT_ARG({'one': 1, 'two': 2, 'three': 3})

#@<> values
# NOTE: TEST() cannot be used to compare dict_values and Dict_values as these do not implement a comparison, and use the default implementation from object
SETUP("values")
EXPECT_EQ(list(expected.values()), list(actual.values()))

SETUP("values", {'a': 1, 'b': 2, 'c': 3})
expected_view = expected.values()
actual_view = actual.values()

EXPECT_EQ(list(expected_view), list(actual_view))
EXPECT_EQ(len(expected_view), len(actual_view))

expected['four'] = 4
actual['four'] = 4

EXPECT_EQ(list(expected_view), list(actual_view))
EXPECT_EQ(len(expected_view), len(actual_view))

#@<> values - members
values_members = [
    "__class__",
    "__delattr__",
    "__dir__",
    "__doc__",
    "__eq__",
    "__format__",
    "__ge__",
    "__getattribute__",
    "__getitem__",
    "__gt__",
    "__hash__",
    "__init__",
    "__init_subclass__",
    "__iter__",
    "__le__",
    "__len__",
    "__lt__",
    "__ne__",
    "__new__",
    "__reduce__",
    "__reduce_ex__",
    "__repr__",
    "__reversed__",
    "__setattr__",
    "__sizeof__",
    "__str__",
    "__subclasshook__",
    "mapping"
]

if sys.hexversion >= 0x030b0000:
    values_members.append("__getstate__")

EXPECT_EQ(sorted(values_members), dir(shdict().values()))

#@<> values - __class__
SETUP_VIEW("values", "__class__")

EXPECT_EQ("dict_values", expected.__class__.__name__)
EXPECT_EQ("Dict_values", actual.__class__.__name__)

#@<> values - __delattr__
SETUP_VIEW("values", "__delattr__")
# __delattr__ comes from the object class
EXPECT_EQ(object.__delattr__, actual.__class__.__delattr__)

#@<> values - __dir__
SETUP_VIEW("values", "__dir__")
all_methods = sorted(actual.__dir__())
# dict_values does not have __getitem__()
all_methods.remove("__getitem__")

if sys.hexversion < 0x030A0000:
    all_methods.remove("mapping")

EXPECT_EQ(sorted(expected.__dir__()), all_methods)

#@<> values - __doc__
SETUP_VIEW("values", "__doc__")
# __doc__ is None
EXPECT_EQ(None, actual.__doc__)
EXPECT_EQ(expected.__doc__, actual.__doc__)

#@<> values - __eq__, __ge__, __gt__, __le__, __lt__, __ne__
# all comparison operators come from the object, the __eq__ and __ne__ are using 'is' operator, the remaining ones are not implemented
ops = [ "__eq__", "__ge__", "__gt__", "__le__", "__lt__", "__ne__"]

for op in ops:
    SETUP_VIEW("values", op)
    EXPECT_EQ(getattr(object, op), getattr(expected.__class__, op))
    EXPECT_EQ(getattr(actual.__class__, op), getattr(expected.__class__, op))

#@<> values - __format__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("values", "__format__")

EXPECT_EQ("Dict_values([])", actual.__format__(""))

SETUP_VIEW("values", "__format__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_values([1, 3, 2])", actual.__format__(""))

# __format__ comes from the object class
EXPECT_EQ(object.__format__, actual.__class__.__format__)

#@<> values - __getattribute__
SETUP_VIEW("values", "__getattribute__")
EXPECT_EQ("__format__", shdict().__getattribute__('__format__').__name__)

#@<> values - __getitem__
SETUP_VIEW("values", "__getitem__")

EXPECT_THROWS(lambda: actual.__getitem__("a"), "TypeError: list indices must be integers or slices, not str")

EXPECT_THROWS(lambda: actual.__getitem__(-1), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(0), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(1), "IndexError: list index out of range")

SETUP_VIEW("values", "__getitem__", {'a': 1, 'b': 2, 'c': 3})

EXPECT_THROWS(lambda: actual.__getitem__(-4), "IndexError: list index out of range")
EXPECT_THROWS(lambda: actual.__getitem__(3), "IndexError: list index out of range")

EXPECT_EQ(1, actual.__getitem__(-3))
EXPECT_EQ(1, actual.__getitem__(0))

EXPECT_EQ(2, actual.__getitem__(-2))
EXPECT_EQ(2, actual.__getitem__(1))

EXPECT_EQ(3, actual.__getitem__(-1))
EXPECT_EQ(3, actual.__getitem__(2))

#@<> values - __hash__
SETUP_VIEW("values", "__hash__")
# __init__ comes from the object class
EXPECT_EQ(object.__hash__, actual.__class__.__hash__)

#@<> values - __init__
SETUP_VIEW("values", "__init__")
# __init__ comes from the object class
EXPECT_EQ(object.__init__, actual.__class__.__init__)

#@<> values - __init_subclass__
SETUP_VIEW("values", "__init_subclass__")
# we should be using default method from object, which throws if it's called with any arguments (correct implementation takes one)
EXPECT_THROWS(lambda: actual.__init_subclass__(1), "__init_subclass__() takes no arguments (1 given)")

#@<> values - __iter__
SETUP_VIEW("values", "__iter__")

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

SETUP_VIEW("values", "__iter__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__iter__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ(1, it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ(2, it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ(3, it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ(1, it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ(2, it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ(3, it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> values - __len__
SETUP_VIEW("values", "__len__")
EXPECT_EQ(list(expected), list(actual))
EXPECT_EQ(expected.__len__(), actual.__len__())

SETUP_VIEW("values", "__len__", {'a': 1, 'b': 2, 'c': 3})
EXPECT_EQ(list(expected), list(actual))
EXPECT_EQ(expected.__len__(), actual.__len__())

#@<> values - __new__
SETUP_VIEW("values", "__new__")
# __new__ comes from the object class
EXPECT_EQ(object.__new__, actual.__class__.__new__)

#@<> values - __reduce__
SETUP_VIEW("values", "__reduce__")
# __reduce__ comes from the object class
EXPECT_EQ(object.__reduce__, actual.__class__.__reduce__)

#@<> values - __reduce_ex__
SETUP_VIEW("values", "__reduce_ex__")
# __reduce_ex__ comes from the object class
EXPECT_EQ(object.__reduce_ex__, actual.__class__.__reduce_ex__)

#@<> values - __repr__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("values", "__repr__")

EXPECT_EQ("Dict_values([])", actual.__repr__())

SETUP_VIEW("values", "__repr__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_values([1, 3, 2])", actual.__repr__())

#@<> values - __reversed__
SETUP_VIEW("values", "__reversed__")

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

SETUP_VIEW("values", "__reversed__", {'a': 1, 'b': 2, 'c': 3})

it = actual.__reversed__()
EXPECT_EQ(3, it.__length_hint__())
p1 = pickle.dumps(it)

EXPECT_EQ(3, it.__next__())
EXPECT_EQ(2, it.__length_hint__())
p2 = pickle.dumps(it)

EXPECT_EQ(2, it.__next__())
EXPECT_EQ(1, it.__length_hint__())
p3 = pickle.dumps(it)

EXPECT_EQ(1, it.__next__())
EXPECT_EQ(0, it.__length_hint__())
p4 = pickle.dumps(it)

EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_THROWS(lambda: it.__next__(), "StopIteration")
EXPECT_EQ(0, it.__length_hint__())
p5 = pickle.dumps(it)

it1 = pickle.loads(p1)
EXPECT_EQ(3, it1.__length_hint__())
EXPECT_EQ(3, it1.__next__())

it2 = pickle.loads(p2)
EXPECT_EQ(2, it2.__length_hint__())
EXPECT_EQ(2, it2.__next__())

it3 = pickle.loads(p3)
EXPECT_EQ(1, it3.__length_hint__())
EXPECT_EQ(1, it3.__next__())

it4 = pickle.loads(p4)
EXPECT_EQ(0, it4.__length_hint__())
EXPECT_THROWS(lambda: it4.__next__(), "StopIteration")

it5 = pickle.loads(p5)
EXPECT_EQ(0, it5.__length_hint__())
EXPECT_THROWS(lambda: it5.__next__(), "StopIteration")

#@<> values - __setattr__
SETUP_VIEW("values", "__setattr__")
# __setattr__ comes from the object class
EXPECT_EQ(object.__setattr__, actual.__class__.__setattr__)

#@<> values - __sizeof__
SETUP_VIEW("values", "__sizeof__")
# __sizeof__ comes from the object class
EXPECT_EQ(object.__sizeof__, actual.__class__.__sizeof__)

#@<> values - __str__
# output is not compared here, Shell is not maintaining the order of insertion, Python is usually using single quotes to quote keys
SETUP_VIEW("values", "__str__")

EXPECT_EQ("Dict_values([])", actual.__str__())

SETUP_VIEW("values", "__str__", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ("Dict_values([1, 3, 2])", actual.__str__())

#@<> values - __subclasshook__
SETUP_VIEW("values", "__subclasshook__")
# __subclasshook__ comes from the object class
EXPECT_EQ(NotImplemented, actual.__class__.__subclasshook__(None))

#@<> values - mapping {sys.hexversion >= 0x030A0000}
SETUP_VIEW("values", "mapping")

EXPECT_EQ(expected.mapping, actual.mapping)

SETUP_VIEW("values", "mapping", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ(expected.mapping, actual.mapping)

#@<> values - mapping - just shdict
SETUP_VIEW("values", "mapping")

EXPECT_EQ("{}", str(actual.mapping))

SETUP_VIEW("values", "mapping", {'one': 1, 'two': 2, 'three': 3})

EXPECT_EQ('{"one": 1, "three": 3, "two": 2}', str(actual.mapping))

EXPECT_THROWS(assign_none_to_mapping, "AttributeError: attribute 'mapping' of 'Dict_values' objects is not writable")

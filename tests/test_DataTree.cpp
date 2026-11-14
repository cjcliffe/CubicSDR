// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "doctest.h"
#include "DataTree.h"

TEST_SUITE("DataElement") {
    TEST_CASE("scalar set and get round-trip with matching types") {
        DataElement e;

        e.set(42);
        CHECK(e.getDataType() == DataElement::Type::DATA_INT);
        int int_val = 0;
        e.get(int_val);
        CHECK(int_val == 42);

        e.set(1.5f);
        CHECK(e.getDataType() == DataElement::Type::DATA_FLOAT);
        float float_val = 0.0f;
        e.get(float_val);
        CHECK(float_val == doctest::Approx(1.5f).epsilon(1e-5));

        e.set(2.5);
        CHECK(e.getDataType() == DataElement::Type::DATA_DOUBLE);
        double double_val = 0.0;
        e.get(double_val);
        CHECK(double_val == doctest::Approx(2.5));

        e.set('a');
        CHECK(e.getDataType() == DataElement::Type::DATA_CHAR);

        e.set(std::string("hello"));
        CHECK(e.getDataType() == DataElement::Type::DATA_STRING);
        std::string str_val;
        e.get(str_val);
        CHECK(str_val == "hello");
    }

    TEST_CASE("additional scalar types round-trip") {
        DataElement e;

        unsigned int u = 0;
        e.set(42u);
        CHECK(e.getDataType() == DataElement::Type::DATA_UINT);
        e.get(u);
        CHECK(u == 42u);

        long lv = 0;
        e.set(42L);
        CHECK(e.getDataType() == DataElement::Type::DATA_LONG);
        e.get(lv);
        CHECK(lv == 42L);

        unsigned long ulv = 0;
        e.set(42UL);
        CHECK(e.getDataType() == DataElement::Type::DATA_ULONG);
        e.get(ulv);
        CHECK(ulv == 42UL);

        long long llv = 0;
        e.set(42LL);
        CHECK(e.getDataType() == DataElement::Type::DATA_LONGLONG);
        e.get(llv);
        CHECK(llv == 42LL);

        unsigned char ucv = 0;
        e.set(static_cast<unsigned char>('b'));
        CHECK(e.getDataType() == DataElement::Type::DATA_UCHAR);
        e.get(ucv);
        CHECK(ucv == static_cast<unsigned char>('b'));
    }

    TEST_CASE("cross-type reads convert stored values") {
        DataElement e;

        e.set(42);
        long long widened = 0;
        e.get(widened);
        CHECK(widened == 42LL);

        e.set(1.5f);
        double as_double = 0.0;
        e.get(as_double);
        CHECK(as_double == doctest::Approx(1.5));

        e.set(42LL);
        int narrowed = 0;
        e.get(narrowed);
        CHECK(narrowed == 42);
    }

    TEST_CASE("remaining scalar vector kinds round-trip") {
        DataElement e;

        const std::vector<char> chars{'a', 'b', 'c'};
        e.set(chars);
        CHECK(e.getDataType() == DataElement::Type::DATA_CHAR_VECTOR);
        std::vector<char> chars_out;
        e.get(chars_out);
        CHECK(chars_out == chars);

        // Regression: get() into vector<unsigned char> must hit the generic
        // overload, not the VOID-buffer one.
        const std::vector<unsigned char> uchars{1, 2, 250};
        e.set(uchars);
        CHECK(e.getDataType() == DataElement::Type::DATA_UCHAR_VECTOR);
        std::vector<unsigned char> uchars_out;
        e.get(uchars_out);
        CHECK(uchars_out == uchars);

        const std::vector<unsigned int> uints{1u, 2u, 4000000000u};
        e.set(uints);
        CHECK(e.getDataType() == DataElement::Type::DATA_UINT_VECTOR);
        std::vector<unsigned int> uints_out;
        e.get(uints_out);
        CHECK(uints_out == uints);

        const std::vector<long> longs{1L, -2L, 3L};
        e.set(longs);
        CHECK(e.getDataType() == DataElement::Type::DATA_LONG_VECTOR);
        std::vector<long> longs_out;
        e.get(longs_out);
        CHECK(longs_out == longs);

        const std::vector<unsigned long> ulongs{1UL, 2UL, 3UL};
        e.set(ulongs);
        CHECK(e.getDataType() == DataElement::Type::DATA_ULONG_VECTOR);
        std::vector<unsigned long> ulongs_out;
        e.get(ulongs_out);
        CHECK(ulongs_out == ulongs);

        const std::vector<long long> llongs{10000000000LL, -20000000000LL};
        e.set(llongs);
        CHECK(e.getDataType() == DataElement::Type::DATA_LONGLONG_VECTOR);
        std::vector<long long> llongs_out;
        e.get(llongs_out);
        CHECK(llongs_out == llongs);

        const std::vector<double> doubles{1.5, -2.5, 3.25};
        e.set(doubles);
        CHECK(e.getDataType() == DataElement::Type::DATA_DOUBLE_VECTOR);
        std::vector<double> doubles_out;
        e.get(doubles_out);
        REQUIRE(doubles_out.size() == doubles.size());
        for (size_t i = 0; i < doubles.size(); i++) {
            CHECK(doubles_out[i] == doctest::Approx(doubles[i]));
        }
    }

    TEST_CASE("long long vectors through DataNode element") {
        DataTree tree("test");
        DataNode *node = tree.rootNode()->newChild("ll");
        const std::vector<long long> in{10000000000LL, -20000000000LL};
        node->element()->set(in);
        CHECK(node->element()->getDataType() == DataElement::Type::DATA_LONGLONG_VECTOR);
        std::vector<long long> out;
        node->element()->get(out);
        CHECK(out == in);
    }

    TEST_CASE("long long vectors convert through DataNode operators") {
        DataTree tree("test");
        DataNode *node = tree.rootNode()->newChild("ll");
        std::vector<long long> in{10000000000LL, -20000000000LL};
        *node = in;
        CHECK(node->element()->getDataType() == DataElement::Type::DATA_LONGLONG_VECTOR);
        std::vector<long long> out = static_cast<std::vector<long long>>(*node);
        CHECK(out == in);
    }

    TEST_CASE("string data pointer exposes stored bytes") {
        DataElement e;
        e.set(std::string("hi"));
        REQUIRE(e.getDataPointer() != nullptr);
        CHECK(std::string(e.getDataPointer(), e.getDataSize()) == "hi");

        DataTree tree("test");
        DataNode *node = tree.rootNode()->newChild("n");
        node->element()->set(42);
        CHECK(static_cast<const char*>(*node) == nullptr);
    }

    TEST_CASE("void buffers round-trip through getVoid") {
        DataElement e;
        const char raw[] = {0x01, 0x02, 0x00, 0x03};
        e.set(raw, sizeof(raw));
        CHECK(e.getDataType() == DataElement::Type::DATA_VOID);
        DataElement::DataElementBuffer out;
        e.getVoid(out);
        REQUIRE(out.size() == sizeof(raw));
        CHECK(std::memcmp(out.data(), raw, sizeof(raw)) == 0);
    }

    TEST_CASE("uchar vectors round-trip through DataNode") {
        DataTree tree("test");
        DataNode *node = tree.rootNode()->newChild("uc");
        const std::vector<unsigned char> in{1, 2, 250};
        node->element()->set(in);
        CHECK(node->element()->getDataType() == DataElement::Type::DATA_UCHAR_VECTOR);
        std::vector<unsigned char> out = static_cast<std::vector<unsigned char>>(*node);
        CHECK(out == in);
    }

    TEST_CASE("truncated buffer guard throws DataException") {
        const DataElement::DataElementBuffer short_buf{0x01, 0x02};
        CHECK_THROWS_AS(DataTreeDetail::readScalar<int>(short_buf), DataException);

        const DataElement::DataElementBuffer empty_buf;
        CHECK_THROWS_AS(DataTreeDetail::readScalar<double>(empty_buf), DataException);

        int expected = 123456;
        DataElement::DataElementBuffer full_buf(sizeof(int));
        std::memcpy(full_buf.data(), &expected, sizeof(int));
        CHECK(DataTreeDetail::readScalar<int>(full_buf) == expected);
    }

    TEST_CASE("vector set and get round-trip") {
        DataElement e;
        const std::vector<int> in{1, 2, 3, 4};
        e.set(in);
        CHECK(e.getDataType() == DataElement::Type::DATA_INT_VECTOR);
        std::vector<int> out;
        e.get(out);
        CHECK(out == in);
    }

    TEST_CASE("string and float vectors round-trip") {
        DataElement e;

        const std::vector<std::string> strings{"alpha", "beta"};
        e.set(strings);
        CHECK(e.getDataType() == DataElement::Type::DATA_STR_VECTOR);
        std::vector<std::string> strings_out;
        e.get(strings_out);
        CHECK(strings_out == strings);

        const std::vector<float> floats{1.5f, 2.5f};
        e.set(floats);
        CHECK(e.getDataType() == DataElement::Type::DATA_FLOAT_VECTOR);
        std::vector<float> floats_out;
        e.get(floats_out);
        REQUIRE(floats_out.size() == floats.size());
        CHECK(floats_out[0] == doctest::Approx(1.5f).epsilon(1e-5));
        CHECK(floats_out[1] == doctest::Approx(2.5f).epsilon(1e-5));
    }

    TEST_CASE("wstring round-trips in memory") {
        DataElement e;
        const std::wstring in = L"hello";
        e.set(in);
        CHECK(e.getDataType() == DataElement::Type::DATA_WSTRING);
        std::wstring out;
        e.get(out);
        CHECK(out == in);
    }

    TEST_CASE("empty wide strings round-trip as empty") {
        DataElement e;
        e.set(std::wstring());
        CHECK(e.getDataType() == DataElement::Type::DATA_WSTRING);
        std::wstring out = L"x";
        e.get(out);
        CHECK(out.empty());
    }

    TEST_CASE("toString narrows wide strings") {
        DataElement e;
        e.set(std::wstring(L"hello"));
        CHECK(e.toString() == "hello");
        e.set(std::wstring());
        CHECK(e.toString().empty());
    }

    TEST_CASE("accessor helpers convert values") {
        DataElement e;
        e.set(42);
        CHECK(e.getInt() == 42);
        CHECK(e.getUInt() == 42u);
        CHECK(e.getLong() == 42L);
        CHECK(e.getULong() == 42UL);
        CHECK(e.getLongLong() == 42LL);
        CHECK(e.getFloat() == doctest::Approx(42.0f).epsilon(1e-5));
        CHECK(e.getDouble() == doctest::Approx(42.0));

        e.set('a');
        CHECK(e.getChar() == 'a');
        CHECK(e.getUChar() == static_cast<unsigned int>('a'));
    }

    TEST_CASE("accessors return zero on unmatched storage") {
        DataElement e;
        e.set(std::string("hi"));
        CHECK(e.getInt() == 0);
        CHECK(e.getUInt() == 0u);
        CHECK(e.getLong() == 0L);
        CHECK(e.getULong() == 0UL);
        CHECK(e.getLongLong() == 0LL);
        CHECK(e.getFloat() == doctest::Approx(0.0f));
        CHECK(e.getDouble() == doctest::Approx(0.0));
        CHECK(e.getChar() == 0);
        CHECK(e.getUChar() == 0u);
    }

    TEST_CASE("empty vectors round-trip as empty") {
        DataElement e;
        const std::vector<int> in;
        e.set(in);
        CHECK(e.getDataType() == DataElement::Type::DATA_INT_VECTOR);
        std::vector<int> out{1, 2, 3};
        e.get(out);
        CHECK(out.empty());
    }

    TEST_CASE("toString formats scalars") {
        DataElement e;
        e.set(42);
        CHECK(e.toString() == "42");
        e.set(std::string("hi"));
        CHECK(e.toString() == "hi");
        e.set(2.5);
        CHECK(e.toString() == "2.500000");
        e.set(1.5f);
        CHECK(e.toString() == "1.500000");
    }

    TEST_CASE("type mismatch raises") {
        DataElement e;
        e.set(42);
        std::string str_val;
        CHECK_THROWS_AS(e.get(str_val), DataTypeMismatchException);

        std::wstring wstr_val;
        CHECK_THROWS_AS(e.get(wstr_val), DataTypeMismatchException);

        std::vector<std::string> strvec_val;
        CHECK_THROWS_AS(e.get(strvec_val), DataTypeMismatchException);

        std::set<std::string> strset_val;
        CHECK_THROWS_AS(e.get(strset_val), DataTypeMismatchException);
    }
}

TEST_SUITE("DataNode") {
    TEST_CASE("children are reachable by name and index") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        root->newChild("a")->element()->set(1);
        root->newChild("b")->element()->set(2);
        root->newChild("a")->element()->set(3);

        CHECK(root->numChildren() == 3);
        CHECK(root->numChildren("a") == 2);
        CHECK(root->child("a")->element()->getInt() == 1);
        CHECK(root->child("a", 1)->element()->getInt() == 3);
        CHECK(root->child(1)->getName() == "b");
    }

    TEST_CASE("names and parent links") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        DataNode *kid = root->newChild("kid");

        CHECK(kid->getName() == "kid");
        CHECK(kid->getParentNode() == root);

        kid->setName("renamed");
        CHECK(kid->getName() == "renamed");
    }

    TEST_CASE("newChild can adopt an existing node") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        DataNode *lone = new DataNode("lone");
        lone->element()->set(5);

        root->newChild("adopted", lone);
        CHECK(root->child("adopted")->element()->getInt() == 5);
        CHECK(root->child("adopted")->getParentNode() == root);
    }

    TEST_CASE("iteration visits every child in order") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        root->newChild("a")->element()->set(1);
        root->newChild("b")->element()->set(2);
        root->newChild("c")->element()->set(3);

        std::vector<std::string> names;
        root->rewind();
        while (root->hasAnother()) {
            names.push_back(root->getNext()->getName());
        }
        REQUIRE(names.size() == 3);
        CHECK(names[0] == "a");
        CHECK(names[1] == "b");
        CHECK(names[2] == "c");
    }

    TEST_CASE("named iteration and rewind") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        root->newChild("a")->element()->set(1);
        root->newChild("b")->element()->set(2);
        root->newChild("a")->element()->set(3);

        std::vector<int> values;
        while (root->hasAnother("a")) {
            values.push_back(root->getNext("a")->element()->getInt());
        }
        REQUIRE(values.size() == 2);
        CHECK(values[0] == 1);
        CHECK(values[1] == 3);

        root->rewind("a");
        CHECK(root->hasAnother("a"));
    }

    TEST_CASE("rewindAll restarts iteration everywhere") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        DataNode *a = root->newChild("a");
        a->newChild("kid")->element()->set(1);
        root->newChild("b")->element()->set(2);

        root->rewind();
        while (root->hasAnother()) {
            DataNode *child = root->getNext();
            child->rewind();
            while (child->hasAnother()) {
                child->getNext();
            }
        }
        CHECK_FALSE(root->hasAnother());

        root->rewindAll();

        std::vector<std::string> names;
        root->rewind();
        while (root->hasAnother()) {
            names.push_back(root->getNext()->getName());
        }
        REQUIRE(names.size() == 2);
        CHECK(names[0] == "a");
        CHECK(names[1] == "b");

        a->rewind();
        REQUIRE(a->hasAnother());
        CHECK(a->getNext()->getName() == "kid");
    }

    TEST_CASE("deep traversal reaches nested children") {
        DataTree tree("test");
        DataNode *level1 = tree.rootNode()->newChild("l1");
        DataNode *level2 = level1->newChild("l2");
        level2->newChild("l3")->element()->set(99);

        CHECK(tree.rootNode()->child("l1")->child("l2")->child("l3")->element()->getInt() == 99);
    }

    TEST_CASE("clone copies the element and its children") {
        DataTree tree("test");
        DataNode *orig = tree.rootNode()->newChild("orig");
        orig->element()->set(7);
        orig->newChild("kid")->element()->set(8);

        DataNode *copy = tree.rootNode()->newChildCloneFrom("copy", orig);
        CHECK(copy->element()->getInt() == 7);
        CHECK(copy->numChildren() == 1);
        CHECK(copy->child("kid")->element()->getInt() == 8);
    }

    TEST_CASE("node clone constructor deep-copies") {
        DataNode orig("orig");
        orig.element()->set(7);
        orig.newChild("kid")->element()->set(8);

        DataNode copy("copy", orig);
        CHECK(copy.element()->getInt() == 7);
        CHECK(copy.numChildren() == 1);
        CHECK(copy.child("kid")->element()->getInt() == 8);
    }

    TEST_CASE("findAll collects every matching descendant") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        root->newChild("a")->element()->set(1);
        DataNode *branch = root->newChild("b");
        branch->newChild("a")->element()->set(2);

        std::vector<DataNode *> found;
        root->findAll("a", found);
        REQUIRE(found.size() == 2);
        CHECK(found[0]->element()->getInt() == 1);
        CHECK(found[1]->element()->getInt() == 2);
    }

    TEST_CASE("findAll with no match returns empty") {
        DataTree tree("test");
        DataNode *root = tree.rootNode();
        root->newChild("a")->element()->set(1);

        std::vector<DataNode *> found;
        root->findAll("missing", found);
        CHECK(found.empty());
    }
}

TEST_SUITE("DataTree") {
    TEST_CASE("save and load round-trip preserves values") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        {
            DataTree tree("config");
            tree.rootNode()->newChild("answer")->element()->set(42);
            tree.rootNode()->newChild("greeting")->element()->set(std::string("hello"));
            tree.rootNode()->newChild("ratio")->element()->set(1.5f);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            CHECK(loaded.rootNode()->getName() == "config");

            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("answer", found);
            REQUIRE(found.size() == 1);
            int answer = 0;
            found[0]->element()->get(answer);
            CHECK(answer == 42);

            found.clear();
            loaded.rootNode()->findAll("greeting", found);
            REQUIRE(found.size() == 1);
            std::string greeting;
            found[0]->element()->get(greeting);
            CHECK(greeting == "hello");

            found.clear();
            loaded.rootNode()->findAll("ratio", found);
            REQUIRE(found.size() == 1);
            float ratio = 0.0f;
            found[0]->element()->get(ratio);
            CHECK(ratio == doctest::Approx(1.5f).epsilon(1e-5));
        }

        std::remove(path.c_str());
    }

    TEST_CASE("load from a missing file returns false") {
        DataTree loaded;
        // The error line on the console is expected; it is the
        // file-not-found diagnostic, not a test failure.
        CHECK_FALSE(loaded.LoadFromFileXML("definitely_not_here_12345.xml"));
    }

    TEST_CASE("floating-point policy controls decoded types") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        {
            DataTree tree("config");
            tree.rootNode()->newChild("ratio")->element()->set(2.5);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree as_float;
            REQUIRE(as_float.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            as_float.rootNode()->findAll("ratio", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_FLOAT);
        }
        {
            DataTree as_double;
            REQUIRE(as_double.LoadFromFileXML(path, USE_DOUBLE));
            std::vector<DataNode *> found;
            as_double.rootNode()->findAll("ratio", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_DOUBLE);
            double ratio = 0.0;
            found[0]->element()->get(ratio);
            CHECK(ratio == doctest::Approx(2.5));
        }

        std::remove(path.c_str());
    }

    TEST_CASE("string vectors round-trip through XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        const std::vector<std::string> in{"alpha", "beta"};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("words")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("words", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_STR_VECTOR);
            std::vector<std::string> out;
            found[0]->element()->get(out);
            CHECK(out == in);
        }

        std::remove(path.c_str());
    }

    TEST_CASE("wide strings round-trip through XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        const std::wstring in = L"hello";
        {
            DataTree tree("config");
            tree.rootNode()->newChild("wide")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("wide", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_WSTRING);
            std::wstring out;
            found[0]->element()->get(out);
            CHECK(out == in);
        }

        std::remove(path.c_str());
    }

    TEST_CASE("integer vectors round-trip through XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        // Values exceed char range so the decoder keeps the int vector type.
        const std::vector<int> in{1000, 2000, 3000};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("big")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("big", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_INT_VECTOR);
            std::vector<int> out;
            found[0]->element()->get(out);
            CHECK(out == in);
        }

        std::remove(path.c_str());
    }

    TEST_CASE("double vectors round-trip through XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        // Exactly representable values so the text form decodes back exactly.
        const std::vector<double> in{1.5, 2.5, 3.5};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("doubles")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path, USE_DOUBLE));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("doubles", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_DOUBLE_VECTOR);
            std::vector<double> out;
            found[0]->element()->get(out);
            REQUIRE(out.size() == in.size());
            for (size_t i = 0; i < in.size(); i++) {
                CHECK(out[i] == doctest::Approx(in[i]));
            }
        }

        std::remove(path.c_str());
    }

    TEST_CASE("char vectors round-trip through XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        const std::vector<char> in{'a', 'b', 'c'};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("letters")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("letters", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_CHAR_VECTOR);
            std::vector<char> out;
            found[0]->element()->get(out);
            CHECK(out == in);
        }

        std::remove(path.c_str());
    }

    TEST_CASE("uchar vector values survive XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        // The typeless XML form cannot decode back to DATA_UCHAR_VECTOR;
        // values are spot-checked instead of the storage type.
        const std::vector<unsigned char> in{1, 2, 250};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("bytes")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("bytes", found);
            REQUIRE(found.size() == 1);
            std::vector<int> out;
            found[0]->element()->get(out);
            REQUIRE(out.size() == in.size());
            for (size_t i = 0; i < in.size(); i++) {
                CHECK(out[i] == in[i]);
            }
        }

        std::remove(path.c_str());
    }

    TEST_CASE("uint vector values survive XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        // Regression: nodeToXML streamed the wrong iterator here, emitting
        // garbage from an empty vector. The typeless XML form cannot decode
        // back to DATA_UINT_VECTOR; values are spot-checked instead.
        const std::vector<unsigned int> in{1000u, 2000u, 3000u};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("counts")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("counts", found);
            REQUIRE(found.size() == 1);
            std::vector<int> out;
            found[0]->element()->get(out);
            REQUIRE(out.size() == in.size());
            for (size_t i = 0; i < in.size(); i++) {
                CHECK(out[i] == static_cast<int>(in[i]));
            }
        }

        std::remove(path.c_str());
    }

    TEST_CASE("float vectors round-trip through XML") {
        const std::string path = "datatree_test_tmp.xml";
        std::remove(path.c_str());

        // Exactly representable values so the text form decodes back exactly.
        const std::vector<float> in{1.5f, 2.5f, 3.5f};
        {
            DataTree tree("config");
            tree.rootNode()->newChild("gains")->element()->set(in);
            CHECK(tree.SaveToFileXML(path));
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("gains", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_FLOAT_VECTOR);
            std::vector<float> out;
            found[0]->element()->get(out);
            REQUIRE(out.size() == in.size());
            for (size_t i = 0; i < in.size(); i++) {
                CHECK(out[i] == doctest::Approx(in[i]).epsilon(1e-5));
            }
        }
        {
            DataTree loaded;
            REQUIRE(loaded.LoadFromFileXML(path, USE_DOUBLE));
            std::vector<DataNode *> found;
            loaded.rootNode()->findAll("gains", found);
            REQUIRE(found.size() == 1);
            CHECK(found[0]->element()->getDataType() == DataElement::Type::DATA_DOUBLE_VECTOR);
            std::vector<double> out;
            found[0]->element()->get(out);
            REQUIRE(out.size() == in.size());
            for (size_t i = 0; i < in.size(); i++) {
                CHECK(out[i] == doctest::Approx(in[i]));
            }
        }

        std::remove(path.c_str());
    }
}

// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include <cstdio>
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

    TEST_CASE("vector set and get round-trip") {
        DataElement e;
        const std::vector<int> in{1, 2, 3, 4};
        e.set(in);
        CHECK(e.getDataType() == DataElement::Type::DATA_INT_VECTOR);
        std::vector<int> out;
        e.get(out);
        CHECK(out == in);
    }

    TEST_CASE("type mismatch raises") {
        DataElement e;
        e.set(42);
        std::string str_val;
        CHECK_THROWS_AS(e.get(str_val), DataTypeMismatchException);
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
}

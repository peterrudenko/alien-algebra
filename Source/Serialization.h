#pragma once

// disabled for now, I'm not really using it:
/*
#include "Common.h"
#include "EGraph.h"
#include "cereal/cereal.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/string.hpp"
#include <sstream>

namespace Serialization
{

struct EGraphDTO final
{
    using Id = e::ClassId;

    struct Term final
    {
        Id leafId;
        String name;
        Vector<Id> childrenIds;

        template <class Archive>
        void serialize(Archive &archive)
        {
            archive(leafId, name, childrenIds);
        }
    };

    struct Class final
    {
        Id classId;
        Vector<Id> termIds;
        Vector<Id> parentIds;

        template <class Archive>
        void serialize(Archive &archive)
        {
            archive(classId, termIds, parentIds);
        }
    };

    Vector<Id> unionFind;
    Vector<Term> terms;
    Vector<Class> classes;

    template <class Archive>
    void serialize(Archive &archive)
    {
        archive(unionFind, terms, classes);
    }
};

static String serialize(const e::Graph &eGraph)
{
    EGraphDTO dto;
    dto.unionFind = eGraph.unionFind.parents;

    for (const auto &[termPtr, leafId] : eGraph.termsLookup)
    {
        dto.terms.push_back({leafId, termPtr->name, termPtr->childrenIds});
    }

    for (const auto &[classId, classPtr] : eGraph.classes)
    {
        EGraphDTO::Class c;
        c.classId = classId;

        for (const auto &term : classPtr->terms)
        {
            c.termIds.push_back(eGraph.termsLookup.at(term));
        }

        // not required at this point, since we won't be modifying the graph any further
        //for (const auto &parent : classPtr->parents)
        //{
        //    c.parentIds.push_back(eGraph.termsLookup.at(parent));
        //}

        dto.classes.push_back(move(c));
    }

    std::stringstream ss;

    {
        cereal::PortableBinaryOutputArchive serializer(ss);
        serializer(dto);
    }

    return ss.str();
}

static e::Graph deserialize(const String &data)
{
    EGraphDTO dto;

    {
        std::stringstream ss(data);
        cereal::PortableBinaryInputArchive deserializer(ss);
        deserializer(dto);
    }

    e::Graph eGraph;
    eGraph.unionFind.parents = move(dto.unionFind);

    HashMap<e::ClassId, e::Term::Ptr> termsLookup;

    for (const auto &term : dto.terms)
    {
        e::Term::Ptr termPtr = make<e::Term>(term.name, term.childrenIds);
        eGraph.termsLookup[termPtr] = term.leafId;
        termsLookup[term.leafId] = termPtr;
    }

    for (const auto &cls : dto.classes)
    {
        auto eClass = make<e::Class>(cls.classId);

        for (const auto &leafId : cls.termIds)
        {
            assert(termsLookup.contains(leafId));
            eClass->terms.push_back(termsLookup[leafId]);
        }

        //for (const auto &leafId : cls.parentIds)
        //{
        //    assert(termsLookup.contains(leafId));
        //    eClass->parents.push_back({termsLookup[leafId], leafId});
        //}

        eGraph.classes[cls.classId] = move(eClass);
    }

    return eGraph;
}

// Base64 encoding/decoding based on https://stackoverflow.com/a/34571089/3158571

static constexpr auto base64symbols = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static String encode(const String &in)
{
    String out;
    unsigned val = 0;
    int valb = -6;

    for (const unsigned char c : in)
    {
        val = (val << 8u) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(base64symbols[(val >> unsigned(valb)) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
    {
        out.push_back(base64symbols[((val << 8u) >> unsigned(valb + 8)) & 0x3F]);
    }

    while (out.size() % 4)
    {
        out.push_back('=');
    }

    return out;
}

static String decode(const String &in)
{
    String out;

    Vector<int> T(256, -1);
    for (int i = 0; i < 64; i++)
    {
        T[base64symbols[i]] = i;
    }

    unsigned val = 0;
    int valb = -8;

    for (const unsigned char c : in)
    {
        if (T[c] == -1)
        {
            break;
        }

        val = (val << 6u) + T[c];
        valb += 6;

        if (valb >= 0)
        {
            out.push_back(char((val >> unsigned(valb)) & 0xFF));
            valb -= 8;
        }
    }

    return out;
}

} // namespace Serialization

*/
module LLIR.RValueBuilder;
import logging;
import LLIR.CllBuilder;

import cpuf.printf;

import hash;
import dstd;
import std;

auto LLIR::RValueBuilder::get() -> LangAPI::RValue {
    return data;
}
auto LLIR::RValueBuilder::deduceType() -> LangAPI::Type {
    LangAPI::Type type = {LangAPI::ValueType::Undef};
    if (value.isString())
        type.type = LangAPI::ValueType::String;
    else if (value.isBoolean())
        type.type = LangAPI::ValueType::Bool;
    else if (value.isNumber())
        type.type = LangAPI::ValueType::Int;
    else if (value.isID()) {
        // likely never execute
        auto find_it = std::find_if(vars.begin(), vars.end(), [&](const LangAPI::Variable &var) { return var.name == value.getID().value; });
        if (find_it == vars.end())
            throw Error("Not found variable to deduce type from expr");
        type = find_it->type;
        undoRuleResult(type.getValueType());
    } else if (value.isAt()) {
        if (unnamed_datablock_units.empty())
            throw Error("no more data accamulated with @");
        const auto &v = unnamed_datablock_units.front();
        type = v.type;
        if (type == LangAPI::ValueType::RuleResult)
            type.type = LangAPI::ValueType::Rule;
        else if (type == LangAPI::ValueType::TokenResult)
            type.type = LangAPI::ValueType::Token;
    } else if (value.isArray()) {
        LangAPI::Type types;
        for (const auto &el : value.getArray().value) {
            if (types == LangAPI::ValueType::Undef) {
                types = CllExprBuilder(*this, el).deduceType();
            } else {
                auto newType = CllExprBuilder(*this, el).deduceType();
                if (newType != types) {
                    type.type = LangAPI::ValueType::Any;
                    break;
                }
            }
        }
        if (types != LangAPI::ValueType::Any) {
            type.template_parameters = {types};
            type.type = LangAPI::ValueType::Array;
        }
    } else if (value.isObject()) {
        // todo: add handle key of different types (int or string)
        LangAPI::Type types = {LangAPI::ValueType::Undef};
        for (const auto &[key, value] : value.getObject().value) {
            if (types == LangAPI::ValueType::Undef) {
                types = CllExprBuilder(*this, value).deduceType();
            } else {
                auto newType = CllExprBuilder(*this, value).deduceType();
                if (newType != types) {
                    type.type = LangAPI::ValueType::Any;
                    break;
                }
            }
        }
        if (types != LangAPI::ValueType::Any) {
            type.template_parameters = {types};
            type.type = LangAPI::ValueType::Map;
        }
    } else {
        throw Error("Undefined rule");
    }
    return type;
}
void LLIR::RValueBuilder::build() {
    if (value.isString()) {
        data.set(LangAPI::String {.value = value.getString().value});
    } else if (value.isBoolean()) {
        data.set(LangAPI::Bool {.value = value.getBoolean().getBoolean()});
    } else if (value.isNumber()) {
        data.set(LangAPI::Float {.value = value.getNumber().getFullNumber()});
    } else if (value.isID()) {
        auto find_it = std::find_if(vars.begin(), vars.end(), [&](const LangAPI::Variable &var) { return var.name == value.getID().value; });
        if (find_it == vars.end())
            throw Error("Not found variable to convert into expr");
        data.set(LangAPI::Symbol {find_it->name});
    } else if (value.isAt()) {
        const auto &v = unnamed_datablock_units.front();
        if (v.type.isValueType() && v.type.getValueType() == LangAPI::ValueType::TokenResult || v.type.getValueType() == LangAPI::ValueType::RuleResult) {
            data.set(LangAPI::StorageSymbol {
                LangAPI::Symbol::createExpression(LangAPI::Symbol {v.name}), stdu::vector<LangAPI::StorageSymbol::PathPart> {LangAPI::IspaLibSymbol {.exports = LangAPI::StdlibExports::MatchResultValue}}
            });
        } else {
            data.set(LangAPI::Symbol {v.name});
        }
    } else if (value.isArray()) {
        LangAPI::Array array;
        LangAPI::Type type;
        utype::unordered_set<LangAPI::Type> variant_types;
        for (const auto el : value.getArray().value) {
            CllExprBuilder builder(*this, el);
            builder.build();
            auto t = builder.deduceType();
            if (type == LangAPI::ValueType::Undef) {
                type = t;
            } else if (type == LangAPI::ValueType::Variant) {
                variant_types.insert(t);
            } else if (type != t) {
                variant_types = {type, t};
                type.type = LangAPI::ValueType::Variant;
            }
            array.values.push_back(builder.get());
        }
        type.template_parameters = decltype(LangAPI::Type::template_parameters) {variant_types.begin(), variant_types.end()};
        array.template_parameters.push_back(std::make_shared<LangAPI::Type>(type));
        data.set(array);
    } else if (value.isObject()) {
        LangAPI::Map map;
        LangAPI::Type key_type = LangAPI::ValueType::String;
        LangAPI::Type value_type = LangAPI::ValueType::Undef;
        utype::unordered_set<LangAPI::Type> value_variant_types;
        for (const auto &[key, value] : value.getObject().value) {
            CllExprBuilder builder(*this, value);
            builder.build();
            auto t = builder.deduceType();
            if (value_type == LangAPI::ValueType::Undef) {
                value_type = t;
            } else if (value_type == LangAPI::ValueType::Variant) {
                value_variant_types.insert(t);
            } else if (value_type != t) {
                value_type.template_parameters = {value_type, t};
                value_type.type = LangAPI::ValueType::Variant;
            }
            map.keys.push_back(LangAPI::String { .value = key });
            map.values.push_back(std::move(builder.get()));
        }
        value_type.template_parameters = decltype(LangAPI::Type::template_parameters) {value_variant_types.begin(), value_variant_types.end()};
        map.template_parameters = { std::make_shared<LangAPI::Type>(key_type), std::make_shared<LangAPI::Type>(value_type) };
        data.set(map);
    } else throw Error("Undefined rvalue");
}

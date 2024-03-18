#pragma once
#include "args.h"
#include "input.h"
#include "spirvcross.h"
#include "bytecode.h"
#include "types/reflection/bindings.h"

namespace shdc::gen {

struct GenInput {
    const Args& args;
    const Input& inp;
    const std::array<Spirvcross,Slang::NUM>& spirvcross;
    const std::array<Bytecode,Slang::NUM>& bytecode;
    const refl::Bindings& merged_bindings;

    GenInput(const Args& args,
             const Input& inp,
             const std::array<Spirvcross,Slang::NUM>& spirvcross,
             const std::array<Bytecode,Slang::NUM>& bytecode,
             const refl::Bindings& merged_bindings);
};

inline GenInput::GenInput(
    const Args& _args,
    const Input& _inp,
    const std::array<Spirvcross,Slang::NUM>& _spirvcross,
    const std::array<Bytecode,Slang::NUM>& _bytecode,
    const refl::Bindings& _merged_bindings):
args(_args),
inp(_inp),
spirvcross(_spirvcross),
bytecode(_bytecode),
merged_bindings(_merged_bindings)
{ };

} // namespace
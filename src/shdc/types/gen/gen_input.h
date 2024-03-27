#pragma once
#include "args.h"
#include "input.h"
#include "spirvcross.h"
#include "bytecode.h"
#include "reflection.h"

namespace shdc::gen {

struct GenInput {
    const Args& args;
    const Input& inp;
    const std::array<Spirvcross,Slang::Num>& spirvcross;
    const std::array<Bytecode,Slang::Num>& bytecode;
    const refl::Reflection& refl;

    GenInput(const Args& args,
             const Input& inp,
             const std::array<Spirvcross,Slang::Num>& spirvcross,
             const std::array<Bytecode,Slang::Num>& bytecode,
             const refl::Reflection& refl);
};

inline GenInput::GenInput(
    const Args& _args,
    const Input& _inp,
    const std::array<Spirvcross,Slang::Num>& _spirvcross,
    const std::array<Bytecode,Slang::Num>& _bytecode,
    const refl::Reflection& _refl):
args(_args),
inp(_inp),
spirvcross(_spirvcross),
bytecode(_bytecode),
refl(_refl)
{ };

} // namespace

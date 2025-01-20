#pragma once
#include "args.h"
#include "input.h"
#include "spirvcross.h"
#include "bytecode.h"
#include "reflection.h"

namespace shdc::gen
{
    struct GenInput
    {
        const Args& args;
        const Input& inp;
        const std::array<Spirvcross, Slang::Num>& spirvcross;
        const std::array<Bytecode, Slang::Num>& bytecode;
        const refl::Reflection& refl;
        const std::vector<uint8_t>& spv_vs;
        const std::vector<uint8_t>& spv_fs;

        GenInput(const Args& args,
                 const Input& inp,
                 const std::array<Spirvcross, Slang::Num>& spirvcross,
                 const std::array<Bytecode, Slang::Num>& bytecode,
                 const refl::Reflection& refl,
                 const std::vector<uint8_t>& spv_vs,
                 const std::vector<uint8_t>& spv_fs);
    };

    inline GenInput::GenInput(
        const Args& _args,
        const Input& _inp,
        const std::array<Spirvcross, Slang::Num>& _spirvcross,
        const std::array<Bytecode, Slang::Num>& _bytecode,
        const refl::Reflection& _refl,
        const std::vector<uint8_t>& spv_vs,
        const std::vector<uint8_t>& spv_fs):
        args(_args),
        inp(_inp),
        spirvcross(_spirvcross),
        bytecode(_bytecode),
        refl(_refl),
        spv_vs(spv_vs),
        spv_fs(spv_fs)
    {
    };
} // namespace

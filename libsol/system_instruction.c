#include "parser.h"
#include <string.h>

#define BAIL_IF(x) {int err = x; if (err) return err;}

enum SystemInstructionKind {
    CreateAccount,
    Assign,
    Transfer,
    CreateAccountWithSeed,
    AdvanceNonceAccount,
    WithdrawNonceAccount,
    InitializeNonceAccount,
    AuthorizeNonceAccount,
    Allocate,
    AllocateWithSeed,
    AssignWithSeed
};

typedef struct SystemTransferInfo {
    Pubkey* from;
    Pubkey* to;
    uint64_t lamports;
} SystemTransferInfo;

static int parse_system_instruction_kind(Parser* parser, enum SystemInstructionKind* kind) {
    return parse_u32(parser, (uint32_t *) kind);
}

static int parse_system_transfer_instruction(Instruction* instruction, Pubkey* pubkeys, size_t pubkeys_length, SystemTransferInfo* info) {
    Parser parser = {instruction->data, instruction->data_length};

    enum SystemInstructionKind kind;
    BAIL_IF(parse_system_instruction_kind(&parser, &kind));
    BAIL_IF(kind != Transfer);
    BAIL_IF(parse_u64(&parser, &info->lamports));

    BAIL_IF(instruction->accounts_length < 2);
    uint8_t from_index = instruction->accounts[0];
    BAIL_IF(from_index >= pubkeys_length);
    info->from = &pubkeys[from_index];

    uint8_t to_index = instruction->accounts[1];
    BAIL_IF(to_index >= pubkeys_length);
    info->to = &pubkeys[to_index];

    return 0;
}

// Returns 0, the fee payer, and system transfer info if provided a transfer message, otherwise non-zero.
int parse_system_transfer_instructions(Parser* parser, MessageHeader* header, SystemTransferInfo* info) {
    BAIL_IF(header->pubkeys_length < 1);
    BAIL_IF(header->instructions_length != 1);

    Instruction instruction;
    BAIL_IF(parse_instruction(parser, &instruction));

    Pubkey* program_id = &header->pubkeys[instruction.program_id_index];
    Pubkey system_program_id = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    BAIL_IF(memcmp(program_id, &system_program_id, PUBKEY_SIZE));

    BAIL_IF(parse_system_transfer_instruction(&instruction, header->pubkeys, header->pubkeys_length, info));

    return 0;
}

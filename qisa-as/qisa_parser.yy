%skeleton "lalr1.cc"
%require  "3.0.4"
%defines
%define api.namespace {QISA}
%define parser_class_name {QISA_Parser}
%define api.token.constructor
%define api.value.type variant
%define parse.assert

 // This code is put in the parser's header file.
%code requires
{
  #include <memory>
  #include <vector>

  namespace QISA
  {
    struct QInstruction
    {
        enum QInstructionType {
          ARG_NONE,
          ARG_ST,
          ARG_TT
        };

        QInstruction(uint64_t p_opcode)
          : type(ARG_NONE)
          , opcode(p_opcode)
          , reg_nr(0xff)
          , is_conditional(false)
        {}

        QInstruction(uint64_t p_opcode,
                     uint8_t p_reg_nr,
                     bool p_is_conditional)
          : type(ARG_ST)
          , opcode(p_opcode)
          , reg_nr(p_reg_nr)
          , is_conditional(p_is_conditional)
        {}

        QInstruction(uint64_t p_opcode,
                     uint8_t p_reg_nr)
          : type(ARG_TT)
          , opcode(p_opcode)
          , reg_nr(p_reg_nr)
          , is_conditional(false)
        {}

        QInstructionType type;
        uint64_t opcode;
        uint8_t reg_nr;
        bool is_conditional;
    };

    typedef std::shared_ptr<QInstruction> QInstructionPtr;
    typedef std::vector<QInstructionPtr> BundledQInstructions;

    class QISA_Driver;
  }
}

%param { QISA::QISA_Driver& driver }
%param { void* flex_scanner }

%locations

%initial-action
{
  // Initialize the initial location.
  @$.begin.filename = @$.end.filename = &driver._filename;
};

// Enable parser tracing and verbose error messages.
// Note that verbose error messages can contain incorrect information.
%define parse.trace
%define parse.error verbose

// This code is put in the parser's implementation file.
%code{
   #include <iostream>
   #include <cstdlib>
   #include <fstream>
   #include <vector>
   #include <utility>

   /* include for all driver functions */
   #include "qisa_driver.h"
}

// To avoid name clashes in the generated files, prefix tokens with TOK_
%define api.token.prefix {TOK_}

/* Standard tokens */
%token               END        0 "end of file"
%token               NEWLINE
%token               COMMA
%token               COLON
%token               VBAR
%token               BRACE_OPEN
%token               BRACE_CLOSE
%token               PAREN_OPEN
%token               PAREN_CLOSE
%token <uint8_t>     Q_REGISTER
%token <uint8_t>     R_REGISTER
%token <uint8_t>     S_REGISTER
%token <uint8_t>     T_REGISTER
%token <std::string> IDENTIFIER
%token <int64_t>     INTEGER
%token <std::string> STRING
%token <std::string> JUNK      /* Unrecognized characters from FLEX. */

/* Assembler directives */
%token DIR_DEF_SYMBOL
%token DIR_REGISTER

/* Branch conditions. */
%token <uint8_t>     COND_ALWAYS
%token <uint8_t>     COND_NEVER
%token <uint8_t>     COND_EQ
%token <uint8_t>     COND_NE
%token <uint8_t>     COND_EQZ
%token <uint8_t>     COND_NEZ
%token <uint8_t>     COND_LT
%token <uint8_t>     COND_LE
%token <uint8_t>     COND_GT
%token <uint8_t>     COND_GE
%token <uint8_t>     COND_LTU
%token <uint8_t>     COND_LEU
%token <uint8_t>     COND_GTU
%token <uint8_t>     COND_GEU
%token <uint8_t>     COND_LTZ
%token <uint8_t>     COND_GEZ
%token <uint8_t>     COND_CARRY
%token <uint8_t>     COND_NOTCARRY


/* Classic mnemonic tokens */
%token NOP
%token STOP
%token ADD
%token SUB
%token ADDC
%token SUBC
%token AND
%token OR
%token XOR
%token NOT
%token CMP
%token BR
%token LDI
%token LDUI
%token FBR
%token FMR
%token SMIS
%token SMIT

/* Quantum instructions that use the same (single) instruction format. */
%token QWAIT
%token QWAITR

/* Aliases, that may result in another or more classic low-level instructions. */
%token SHL1
%token NAND
%token NOR
%token XNOR
%token BRA
%token GOTO
%token BRN
%token BEQ
%token BNE
%token BLT
%token BLE
%token BGT
%token BGE
%token BLTU
%token BLEU
%token BGTU
%token BGEU
%token COPY
%token MOV
%token MULT2

/* Bundle separator */
%token BS

/* Specifies a conditional single qubit quantum instruction. */
%token COND_Q_INSTR_ST

/* Type mappings. */

// Label is currently not used, instead an offset to a label is used.
//
// %type <int64_t>                                       label

%type <int64_t>                                       offset_to_label
%type <int64_t>                                       imm
%type <uint8_t>                                       q_reg
%type <uint8_t>                                       r_reg
%type <uint8_t>                                       s_reg
%type <uint8_t>                                       t_reg
%type <uint8_t>                                       cond
%type <std::pair<uint8_t, uint8_t> >                  target_control_pair
%type <std::vector<uint8_t> >                         one_or_more_qubit_addresses s_mask
%type <std::vector<std::pair<uint8_t, uint8_t> > >    one_or_more_control_target_pairs t_mask
%type <uint8_t>                                       q_bs

%type <QISA::QInstructionPtr>                         q_instr q_instr_arg_none q_instr_arg_one
%type <QISA::BundledQInstructions>                    one_or_more_q_instrs

%%

/* The assembler grammar */

/* A program is defined recursively as either an instruction
 * or a program followed by an instruction. */

program
  : program instruction
  | instruction
  ;

instruction
  : NEWLINE
  | definition    NEWLINE
  | register_decl NEWLINE
  | label_decl    NEWLINE
  | statement     NEWLINE
  | label_decl statement NEWLINE
  | JUNK
    {
      driver.error(@1, "Illegal input detected.");
      YYABORT;
    }
  ;

REGISTER
  : Q_REGISTER
  | R_REGISTER
  | S_REGISTER
  | T_REGISTER
  | error
    {
      driver.addExpectationErrorMessage("a register");
      YYABORT;
    }
  ;


  /* assembler directives */
definition
  : DIR_DEF_SYMBOL IDENTIFIER INTEGER { driver.add_symbol($2, @2, $3, @3); }
  | DIR_DEF_SYMBOL IDENTIFIER STRING  { driver.add_symbol($2, @2, $3, @3); }
  ;
register_decl
  : DIR_REGISTER Q_REGISTER IDENTIFIER
    {
      if (!driver.add_register_definition($3, @3, $2, @2, QISA::QISA_Driver::Q_REGISTER))
      {
        YYERROR;
      };
    }
  | DIR_REGISTER R_REGISTER IDENTIFIER
    {
      if (!driver.add_register_definition($3, @3, $2, @2, QISA::QISA_Driver::R_REGISTER))
      {
        YYERROR;
      };
    }
  | DIR_REGISTER S_REGISTER IDENTIFIER
    {
      if (!driver.add_register_definition($3, @3, $2, @2, QISA::QISA_Driver::S_REGISTER))
      {
        YYERROR;
      };
    }
  | DIR_REGISTER T_REGISTER IDENTIFIER
    {
      if (!driver.add_register_definition($3, @3, $2, @2, QISA::QISA_Driver::T_REGISTER))
      {
        YYERROR;
      };
    }
  | DIR_REGISTER REGISTER error
    {
       driver.addSpecificErrorMessage("Note that you cannot use an existing register or instruction name as register alias");
       YYABORT;
    }
  ;

label_decl
  : IDENTIFIER COLON { driver.add_label($1, @1); }
  ;

// Label is currently not used, instead an offset to a label is used.
//
// /* a label is a reference to a memory address, a constant is also valid. */
// label
//   : INTEGER { $$ = $1; }
//   | IDENTIFIER
//     { int64_t addr = driver.get_label_address($1, @1, false);
//       $$ = addr;
//     }
//   ;

/* offset_to_label represents an offset from the current program counter.
 * A constant is also valid. */
offset_to_label
  : INTEGER { $$ = $1; }
  | IDENTIFIER
    { int64_t addr = driver.get_label_address($1, @1, true);
      $$ = addr;
    }
  ;

/* An x_reg is either a register specification or a reference to a defined register 'symbol' */
q_reg
  : Q_REGISTER { $$ = $1; }
  | IDENTIFIER
    {
      uint8_t reg_nr;
      bool success = driver.get_register_nr($1, @1, QISA::QISA_Driver::Q_REGISTER, reg_nr);
      if (success)
      {
        $$ = reg_nr;
      }
      else
      {
        YYERROR;
      }
    }
  | error
    {
      driver.addExpectationErrorMessage("a Q_REGISTER");
      YYABORT;
    }
  ;

r_reg
  : R_REGISTER { $$ = $1; }
  | IDENTIFIER
    {
      uint8_t reg_nr;
      bool success = driver.get_register_nr($1, @1, QISA::QISA_Driver::R_REGISTER, reg_nr);
      if (success)
      {
        $$ = reg_nr;
      }
      else
      {
        YYERROR;
      }
    }
  | error
    {
      driver.addExpectationErrorMessage("an R_REGISTER");
      YYABORT;
    }
  ;

s_reg
  : S_REGISTER { $$ = $1; }
  | IDENTIFIER
    {
      uint8_t reg_nr;
      bool success = driver.get_register_nr($1, @1, QISA::QISA_Driver::S_REGISTER, reg_nr);
      if (success)
      {
        $$ = reg_nr;
      }
      else
      {
        YYERROR;
      }
    }
  | error
    {
      driver.addExpectationErrorMessage("an S_REGISTER");
      YYABORT;
    }
  ;

t_reg
  : T_REGISTER { $$ = $1; }
  | IDENTIFIER
    {
      uint8_t reg_nr;
      bool success = driver.get_register_nr($1, @1, QISA::QISA_Driver::T_REGISTER, reg_nr);
      if (success)
      {
        $$ = reg_nr;
      }
      else
      {
        YYERROR;
      }
    }
  | error
    {
      driver.addExpectationErrorMessage("a T_REGISTER");
      YYABORT;
    }
  ;

  /* An immediate value is a constant or a definition */
imm
  : INTEGER { $$ = $1; }
  | IDENTIFIER
    {
      int64_t imm_val;
      bool success = driver.get_symbol($1, @1, imm_val);
      if (success)
      {
        $$ = imm_val;
      }
      else
      {
        YYERROR;
      }
    }
  | error
    {
      driver.addExpectationErrorMessage("an IMMEDIATE VALUE");
      YYABORT;
    }
  ;

/* Branch conditions */
cond
  : COND_ALWAYS
    { $$ = $1; }
  | COND_NEVER
    { $$ = $1; }
  | COND_EQ
    { $$ = $1; }
  | COND_NE
    { $$ = $1; }
  | COND_LT
    { $$ = $1; }
  | COND_LE
    { $$ = $1; }
  | COND_GT
    { $$ = $1; }
  | COND_GE
    { $$ = $1; }
  | COND_LTU
    { $$ = $1; }
  | COND_LEU
    { $$ = $1; }
  | COND_GTU
    { $$ = $1; }
  | COND_GEU
    { $$ = $1; }
  | COND_LTZ
    { $$ = $1; }
  | COND_GEZ
    { $$ = $1; }
  | COND_EQZ
    { $$ = $1; }
  | COND_NEZ
    { $$ = $1; }
  | COND_CARRY
    { $$ = $1; }
  | COND_NOTCARRY
    { $$ = $1; }
  | error
    {
      driver.addExpectedConditionErrorMessage();
      YYABORT;
    }
  ;

one_or_more_qubit_addresses
  : INTEGER
    {
      if (driver.validate_qubit_address($1, @1))
      {
        uint8_t number = $1;
        $$ = std::vector<uint8_t>();
        $$.push_back(number);
      }
      else
      {
        YYERROR;
      }
    }
  | one_or_more_qubit_addresses COMMA INTEGER
    {
      if (driver.validate_qubit_address($3, @3))
      {
        uint8_t number = $3;
        std::vector<uint8_t> &args = $1;
        args.push_back(number);
        $$ = args;
      }
      else
      {
        YYERROR;
      }
    }
  ;

s_mask
  : BRACE_OPEN one_or_more_qubit_addresses BRACE_CLOSE
    {
      if (driver.validate_s_mask($2, @$))
      {
        $$ = $2;
      }
      else
      {
        YYERROR;
      }
    }
  ;

target_control_pair
  : PAREN_OPEN INTEGER COMMA INTEGER PAREN_CLOSE
    {
      auto tc_pair = std::make_pair($2, $4);

      if (driver.validate_target_control_pair(tc_pair, @$))
      {
        $$ = tc_pair;
      }
      else
      {
        YYERROR;
      }
    }
  ;

one_or_more_control_target_pairs
  : target_control_pair
    {
      std::pair<uint8_t,uint8_t> first_pair = $1;
      $$ = std::vector<std::pair<uint8_t,uint8_t> >();
      $$.push_back(first_pair);
    }
  | one_or_more_control_target_pairs COMMA target_control_pair
    {
      std::pair<uint8_t,uint8_t> next_pair = $3;
      std::vector<std::pair<uint8_t,uint8_t> > &pairs = $1;
      pairs.push_back(next_pair);
      $$ = pairs;
    }
  ;

t_mask
  : BRACE_OPEN one_or_more_control_target_pairs BRACE_CLOSE
    {
      if (driver.validate_t_mask($2, @$))
      {
        $$ = $2;
      }
      else
      {
        YYERROR;
      }
    }
  ;


q_instr_arg_none
  : IDENTIFIER
    {
      QISA::QInstructionPtr
      instr = driver.get_q_instr_arg_none($1, @1);
      if (instr)
      {
        $$ = instr;
      }
      else
      {
        YYERROR;
      }
    }

q_instr_arg_one
  : IDENTIFIER S_REGISTER
    {
      QISA::QInstructionPtr
      instr = driver.get_q_instr_arg_st($1, @1, $2, @2, false);
      if (instr)
      {
        $$ = instr;
      }
      else
      {
        YYERROR;
      }
    }
  |  COND_Q_INSTR_ST IDENTIFIER s_reg
     {
       QISA::QInstructionPtr
       instr = driver.get_q_instr_arg_st($2, @2, $3, @3, true);
       if (instr)
       {
         $$ = instr;
       }
       else
       {
         YYERROR;
       }
     }

  | IDENTIFIER T_REGISTER
    {
      QISA::QInstructionPtr
      instr = driver.get_q_instr_arg_tt($1, @1, $2, @2);
      if (instr)
      {
        $$ = instr;
      }
      else
      {
        YYERROR;
      }
    }

  | IDENTIFIER IDENTIFIER
    {
      /* The second identifier can either represent an s_register or a t_register.
         Let the driver decide. */
      QISA::QInstructionPtr
      instr = driver.get_q_instr_arg_one($1, @1, $2, @2);
      if (instr)
      {
        $$ = instr;
      }
      else
      {
        YYERROR;
      }
    }

q_instr
  : q_instr_arg_none
    { $$ = $1; }
  | q_instr_arg_one
    { $$ = $1; }
  | error
    {
       driver.addExpectationErrorMessage("a valid quantum instruction");
       YYABORT;
    }
  ;

one_or_more_q_instrs
  : q_instr
    {
      $$ = QISA::BundledQInstructions();
      $$.push_back($1);
      QInstructionPtr inst = $1;
    }
  | one_or_more_q_instrs VBAR q_instr
    {
      QISA::BundledQInstructions &instrs = $1;
      instrs.push_back($3);
      $$ = instrs;
    }
    ;

// Make the BS token optional.
optional_bs
  :
  | BS
  ;


// Bundle separator
q_bs
  : optional_bs INTEGER
    {
      if (driver.validate_bundle_separator($2, @2))
      {
        $$ = (uint8_t)$2;
      }
      else
      {
        YYERROR;
      }
    }
 // This is to cope when a bundle starts with BS, but does not specify the wait time.
 // In this case, we use a default wait time of 1.
  | BS
    {
      $$ = 1;
    }
  ;


/* Grammars for each instruction */

/* The quantum bundle specification used some state that had to be cleared
 * whenever classic instructions were encountered.
 * We therefore had to split statement into a classic statement and a quantum statement.
 * We leave it split now, for these historic reasons.
 */

statement
 : classic_statement
 | quantum_statement
 ;

classic_statement

  /************************
   * CLASSIC INSTRUCTIONS *
   ************************/

  /* nop */
  : NOP { if (!driver.generate_NOP(@1)) { YYERROR;} }
  /* stop */
  | STOP { if (!driver.generate_STOP(@1)) { YYERROR;} }
  /* add rd, rs, rt */
  | ADD r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("ADD", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* sub rd, rs, rt */
  | SUB r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("SUB", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* addc rd, rs, rt */
  | ADDC r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("ADDC", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* subc rd, rs, rt */
  | SUBC r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("SUBC", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* and rd, rs, rt */
  | AND r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("AND", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* or rd, rs, rt */
  | OR r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("OR", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* xor rd, rs, rt */
  | XOR r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XXX_rd_rs_rt("XOR", @1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }
  /* not rd, rt */
  | NOT r_reg COMMA r_reg { if (!driver.generate_NOT(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* cmp rs,rt */
  | CMP r_reg COMMA r_reg { if (!driver.generate_CMP(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* br cond, addr */
  | BR cond COMMA offset_to_label { if (!driver.generate_BR(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* ldi rd, imm */
  | LDI r_reg COMMA imm { if (!driver.generate_LDI(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* ldui rd, u_imm */
  | LDUI r_reg COMMA imm { if (!driver.generate_LDUI(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* fbr cond, rd */
  | FBR cond COMMA r_reg { if (!driver.generate_FBR(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* fmr rd, qs */
  | FMR r_reg COMMA q_reg { if (!driver.generate_FMR(@1, $2, @2, $4, @4)) { YYERROR;} }

  /* smis sd, s_mask */
  | SMIS s_reg COMMA s_mask { if (!driver.generate_SMIS(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* smis sd, imm  (NOTE: Alternative representation.) */
  | SMIS s_reg COMMA INTEGER { if (!driver.generate_SMIS(@1, $2, @2, $4, @4)) { YYERROR;} }

  /* smit td, t_mask */
  | SMIT t_reg COMMA t_mask { if (!driver.generate_SMIT(@1, $2, @2, $4, @4)) { YYERROR;} }
  /* smit td, imm  (NOTE: Alternative representation.) */
  | SMIT t_reg COMMA INTEGER { if (!driver.generate_SMIT(@1, $2, @2, $4, @4)) { YYERROR;} }

  /* qwait u_imm */
  | QWAIT imm { if (!driver.generate_QWAIT(@1, $2, @2)) { YYERROR;} }

  /* qwaitr rs */
  | QWAITR r_reg { if (!driver.generate_QWAITR(@1, $2, @2)) { YYERROR;} }

  /***********
   * ALIASES *
   ***********/

  /* SHL1  rd, rs        */
  | SHL1 r_reg COMMA r_reg { if (!driver.generate_SHL1(@1, $2, @2, $4, @4)) { YYERROR;} }

  /* NAND  rd, rs, rt    */
  | NAND r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_NAND(@1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }

  /* NOR   rd, rs, rt    */
  | NOR r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_NOR(@1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }

  /* XNOR  rd, rs, rt    */
  | XNOR r_reg COMMA r_reg COMMA r_reg { if (!driver.generate_XNOR(@1, $2, @2, $4, @4, $6, @6)) { YYERROR;} }

  /* BRA   addr          */
  | BRA offset_to_label { if (!driver.generate_BRA(@1, $2, @2)) { YYERROR;} }

  /* GOTO addr          */
  | GOTO offset_to_label { if (!driver.generate_GOTO(@1, $2, @2)) { YYERROR;} }

  /* BRN   addr          */
  | BRN offset_to_label { if (!driver.generate_BRN(@1, $2, @2)) { YYERROR;} }

  /* BEQ   rs, rt, addr  */
  | BEQ r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_EQ)) { YYERROR;} }

  /* BNE   rs, rt, addr  */
  | BNE r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_NE)) { YYERROR;} }

  /* BLT   rs, rt, addr  */
  | BLT r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_LT)) { YYERROR;} }

  /* BLE   rs, rt, addr  */
  | BLE r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_LE)) { YYERROR;} }

  /* BGT   rs, rt, addr  */
  | BGT r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_GT)) { YYERROR;} }

  /* BGE   rs, rt, addr  */
  | BGE r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_GE)) { YYERROR;} }

  /* BLTU  rs, rt, addr  */
  | BLTU r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_LTU)) { YYERROR;} }

  /* BLEU  rs, rt, addr  */
  | BLEU r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_LEU)) { YYERROR;} }

  /* BGTU  rs, rt, addr  */
  | BGTU r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_GTU)) { YYERROR;} }

  /* BGEU  rs, rt, addr  */
  | BGEU r_reg COMMA r_reg COMMA offset_to_label
    { if (!driver.generate_BR_COND(@1, $2, @2, $4, @4, $6, @6, QISA::QISA_Driver::COND_GEU)) { YYERROR;} }

  /* COPY  rd, rs        */
  | COPY r_reg COMMA r_reg
    { if (!driver.generate_COPY(@1, $2, @2, $4, @4)) { YYERROR;} }

  /* MOV   rd, imm        */
  | MOV r_reg COMMA imm
    { if (!driver.generate_MOV(@1, $2, @2, $4, @4)) { YYERROR;} }

  /* MULT2 rd, rs        */
  | MULT2 r_reg COMMA r_reg
    { if (!driver.generate_MULT2(@1, $2, @2, $4, @4)) { YYERROR;} }

  ;

  /************************
   * QUANTUM INSTRUCTIONS *
   ************************/
quantum_statement
  /* Bundle of quantum instructions. */
  : q_bs one_or_more_q_instrs { if (!driver.generate_q_bundle($1, @1, $2, @2)) { YYABORT;} }
  // If a bundle specification is not given, use a default of 1.
  | one_or_more_q_instrs { if (!driver.generate_q_bundle(1, @$, $1, @1)) { YYABORT;} }
  ;
%%

// Error member function passes the errors to the driver.
void
QISA::QISA_Parser::error( const location_type &l, const std::string &err_message )
{
  driver.error(l, err_message);
}

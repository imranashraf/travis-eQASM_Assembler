#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>

#include "qisa_driver.h"
#include "qisa_version.h"

#include "qisa_opcode_defs.inc"

#include "qisa_qmap_parser.h"

namespace QISA
{

// Set the prefix that denotes a label in the disassembly.
const char* QISA_Driver::DISASSEMBLY_LABEL_PREFIX = "label_";

QISA_Driver::QISA_Driver()
    : _traceScanning(false)
    , _traceParsing(false)
    , _verbose(false)
    , _hadEOF(false)
    , _totalNrOfQubits(0)
    , _max_bs_val(0)
    , _disassemblyFormatId(1)
    , _disassemblyLabelStringLength(0)
    , _disassemblyStartedQuantumBundle(false)
    , _maxQuantumOpcodeVal(Q_INST_OPCODE_MASK) // 8 bits for the quantum instruction opcode.
    , _lastDriverAction(DRIVER_ACTION_NONE)
{
  // Bring in the opcodes that have been defined for the qisa instructions.
  setOpcodes();

  // Number of registers per kind of register.
  _nrOfRegisters[Q_REGISTER] = 7;
  _nrOfRegisters[R_REGISTER] = 32;
  _nrOfRegisters[S_REGISTER] = 32;
  _nrOfRegisters[T_REGISTER] = 64;

  _registerName[Q_REGISTER] = 'Q';
  _registerName[R_REGISTER] = 'R';
  _registerName[S_REGISTER] = 'S';
  _registerName[T_REGISTER] = 'T';

  _totalNrOfQubits = 0;

  // Maximum value to specify as bundle separator.
  // The width of this field is currently 3 bits, so the maximum value is set to 7.
  _max_bs_val = 7;

  // Name the branch conditions.
  _branchConditionNames[COND_ALWAYS] = "ALWAYS";
  _branchConditionNames[COND_NEVER ] = "NEVER";
  _branchConditionNames[COND_EQ    ] = "EQ";
  _branchConditionNames[COND_NE    ] = "NE";
  _branchConditionNames[COND_LTZ   ] = "LTZ";
  _branchConditionNames[COND_GEZ   ] = "GEZ";
  _branchConditionNames[COND_LTU   ] = "LTU";
  _branchConditionNames[COND_GEU   ] = "GEU";
  _branchConditionNames[COND_LEU   ] = "LEU";
  _branchConditionNames[COND_GTU   ] = "GTU";
  _branchConditionNames[COND_LT    ] = "LT";
  _branchConditionNames[COND_GE    ] = "GE";
  _branchConditionNames[COND_LE    ] = "LE";
  _branchConditionNames[COND_GT    ] = "GT";

  // Branch condition aliases.
  // Maps the alias name to the 'real' condition.
  _branchConditionAliases["NOTCARRY"] = 0x8;
  _branchConditionAliases["CARRY"]    = 0x9;
  _branchConditionAliases["EQZ"]      = 0xa;
  _branchConditionAliases["NEZ"]      = 0xb;

  /*
  // Valid target-control pairs: 'left-to-right' direction
  _valid_target_control_pairs[std::make_pair(2,0)] =  0;
  _valid_target_control_pairs[std::make_pair(0,3)] =  1;
  _valid_target_control_pairs[std::make_pair(4,1)] =  2;
  _valid_target_control_pairs[std::make_pair(1,5)] =  3;
  _valid_target_control_pairs[std::make_pair(5,2)] =  4;
  _valid_target_control_pairs[std::make_pair(2,6)] =  5;
  _valid_target_control_pairs[std::make_pair(6,3)] =  6;
  _valid_target_control_pairs[std::make_pair(4,7)] =  7;
  _valid_target_control_pairs[std::make_pair(7,5)] =  8;
  _valid_target_control_pairs[std::make_pair(5,8)] =  9;
  _valid_target_control_pairs[std::make_pair(8,6)] =  10;
  _valid_target_control_pairs[std::make_pair(6,9)] =  11;
  _valid_target_control_pairs[std::make_pair(7,10)] =  12;
  _valid_target_control_pairs[std::make_pair(10,8)] =  13;
  _valid_target_control_pairs[std::make_pair(8,11)] =  14;
  _valid_target_control_pairs[std::make_pair(11,9)] =  15;
  _valid_target_control_pairs[std::make_pair(9,12)] =  16;
  _valid_target_control_pairs[std::make_pair(13,10)] =  17;
  _valid_target_control_pairs[std::make_pair(10,14)] =  18;
  _valid_target_control_pairs[std::make_pair(14,11)] =  19;
  _valid_target_control_pairs[std::make_pair(11,15)] =  20;
  _valid_target_control_pairs[std::make_pair(15,12)] =  21;
  _valid_target_control_pairs[std::make_pair(13,16)] =  22;
  _valid_target_control_pairs[std::make_pair(16,14)] =  23;

  // Valid target-control pairs: opposite direction
  _valid_target_control_pairs[std::make_pair(0,2)] =  24;
  _valid_target_control_pairs[std::make_pair(3,0)] =  25;
  _valid_target_control_pairs[std::make_pair(1,4)] =  26;
  _valid_target_control_pairs[std::make_pair(5,1)] =  27;
  _valid_target_control_pairs[std::make_pair(2,5)] =  28;
  _valid_target_control_pairs[std::make_pair(6,2)] =  29;
  _valid_target_control_pairs[std::make_pair(3,6)] =  30;
  _valid_target_control_pairs[std::make_pair(7,4)] =  31;
  _valid_target_control_pairs[std::make_pair(5,7)] =  32;
  _valid_target_control_pairs[std::make_pair(8,5)] =  33;
  _valid_target_control_pairs[std::make_pair(6,8)] =  34;
  _valid_target_control_pairs[std::make_pair(9,6)] =  35;
  _valid_target_control_pairs[std::make_pair(10,7)] =  36;
  _valid_target_control_pairs[std::make_pair(8,10)] =  37;
  _valid_target_control_pairs[std::make_pair(11,8)] =  38;
  _valid_target_control_pairs[std::make_pair(9,11)] =  39;
  _valid_target_control_pairs[std::make_pair(12,9)] =  40;
  _valid_target_control_pairs[std::make_pair(10,13)] =  41;
  _valid_target_control_pairs[std::make_pair(14,10)] =  42;
  _valid_target_control_pairs[std::make_pair(11,14)] =  43;
  _valid_target_control_pairs[std::make_pair(15,11)] =  44;
  _valid_target_control_pairs[std::make_pair(12,15)] =  45;
  _valid_target_control_pairs[std::make_pair(16,13)] =  46;
  _valid_target_control_pairs[std::make_pair(14,16)] =  47;
  */

  // Lookup table to go from bit number to target-control pair.
  _bit2tc_pair[ 0] = std::make_pair(2,0);
  _bit2tc_pair[ 1] = std::make_pair(0,3);
  _bit2tc_pair[ 2] = std::make_pair(3,1);
  _bit2tc_pair[ 3] = std::make_pair(1,4);
  _bit2tc_pair[ 4] = std::make_pair(2,5);
  _bit2tc_pair[ 5] = std::make_pair(5,3);
  _bit2tc_pair[ 6] = std::make_pair(3,6);
  _bit2tc_pair[ 7] = std::make_pair(6,4);
  _bit2tc_pair[ 8] = std::make_pair(0,2);
  _bit2tc_pair[ 9] = std::make_pair(3,0);
  _bit2tc_pair[10] = std::make_pair(1,3);
  _bit2tc_pair[11] = std::make_pair(4,1);
  _bit2tc_pair[12] = std::make_pair(5,2);
  _bit2tc_pair[13] = std::make_pair(3,5);
  _bit2tc_pair[14] = std::make_pair(6,3);
  _bit2tc_pair[15] = std::make_pair(4,6);

}

void
QISA_Driver::read(std::string input_filename)
{
    std::ifstream                           in;
    //std::string                             input_filename;
    std::string                            line;

    std::pair <int, int> one[48];
    std::pair <uint8_t, uint8_t> two[48];
    int             i = 0;
    int             qubit_num = 0;
    int             num_edge_address = 0;
    bool read_qubit_num, read_edge_list, read_num_nn_edge = false;

    //input_filename = "D:\\Projects\\QuMA_Sim\\scripts\\quantum_layout_information.txt";

    in.open(input_filename);
    //std::cout << "opening file..." << std::endl;

    if (!in) {
        std::cerr << "Error while open quantum layout information file" << std::endl;
    }

    while (getline(in, line))
    {
        std::istringstream iss(line);
        int num_qubits;
        int edge_number, left_operation, right_operation;
        char buff;

        //std::cout << line << std::endl;
        /***********************Read number of qubits****************************/
        if (strcmp(line.c_str(), ".EndNumQubits") == 0) {
            read_qubit_num = false;
            //std::cout << "End reading number of qubits" << std::endl;
        }

        if (read_qubit_num == true) {
            iss >> num_qubits;
            qubit_num = num_qubits;
            //std::cout << "There are " << num_qubits << " qubits" << std::endl;
        }

        if (strcmp(line.c_str(), ".NumQubits") == 0) {
            read_qubit_num = true;
            //std::cout << "Start reading number of qubits" << std::endl;
        }

        /***********************Read number of direct edge address*********************/
        if (strcmp(line.c_str(), ".EndNumDirEdge") == 0) {
            read_num_nn_edge = false;
            //std::cout << "End reading number of direct edge address" << std::endl;
        }

        if (read_num_nn_edge == true) {
            iss >> num_edge_address;
            //std::cout << "There are " << num_edge_address << " edge addresses there" << std::endl;
        }

        if (strcmp(line.c_str(), ".NumDirEdge") == 0) {
            read_num_nn_edge = true;
            //std::cout << "\nStart reading number of direct edge address" << std::endl;
        }

        /***********************Read edge list information***********************/
        if (strcmp(line.c_str(), ".EndEdgeList") == 0) {
            read_edge_list = false;
            //std::cout << "End reading edge list" << std::endl;
        }

        if (read_edge_list == true) {
            if (!(iss >> edge_number >> buff >> left_operation >> buff >> right_operation)) { break; } // error

            iss >> edge_number >> buff >> left_operation >> buff >> right_operation;
            //if (!check_range(left_operation, min, max) {

            //}
            uint8_t a = static_cast<uint8_t>(left_operation);
            uint8_t b = right_operation;
            //std::cout << a << b << std::endl;
            //printf("%d, %d\n", a, b);
            one[i] = std::make_pair(left_operation, right_operation);
            two[i] = std::make_pair(a, b);
            //std::cout << one[i].first << ", " << one[i].second << std::endl;
            i++;
        }

        if (strcmp(line.c_str(), ".EdgeList") == 0) {
            read_edge_list = true;
            //std::cout << "Start reading edge list" << std::endl;
        }

    }

    in.close();

    //initiate target_control_pairs
    for (int j = 0; j < num_edge_address; j++) {
        _valid_target_control_pairs[two[j]] = j;
    }
    _totalNrOfQubits = qubit_num;
    _NrOfEdgeAdress = num_edge_address;

    //In the QCC assembler, one binary instruction generated for SMIS and three binary instructions generated for SMIT
    pos_number_s = 1;
    pos_number_t = 3;

}

void
QISA_Driver::reset()
{
  _hadEOF = false;
  _filename.clear();

  _instructions.clear();

  _disassembledInstructions.clear();

  _disassemblyStartedQuantumBundle = false;

  _disassemblyLabels.clear();
  _labels.clear();

  _registerAliases[0].clear();
  _registerAliases[1].clear();
  _registerAliases[2].clear();
  _registerAliases[3].clear();

  _intSymbols.clear();

  _strSymbols.clear();

  _deferredInstructions.clear();

  _errorStream.str(""); // Clear the accumulated error messages.
  _errorStream.clear(); // Clear state flags.

  _lastDriverAction = DRIVER_ACTION_NONE;

  _errorLoc = location();
}

bool
QISA_Driver::assemble(const std::string &filename)
{
  // First, reset the driver to get a clean start.
  reset();

  yyscan_t flex_scanner;

  _filename = filename;
  bool success = scanBegin(&flex_scanner);

  if (!success)
  {
    return false;
  }

  if(_totalNrOfQubits == 0) {
    std::cout << "\nError: the quantum layout information is not read into assembler" << std::endl;
    success = false;
  }

  QISA_Parser parser (*this, flex_scanner);
  parser.set_debug_level (_traceParsing);

  int parser_result = parser.parse ();
  scanEnd(flex_scanner);

  if (parser_result == 0)
  {
    success = processDeferredInstructions();
  }
  else
  {
    success = false;
  }

  // This is for save() to know it has to save binary assembly output.
  _lastDriverAction = DRIVER_ACTION_PARSE;
  return success;
}

bool
QISA_Driver::disassemble(const std::string& filename)
{
  // First reset the driver to get a clean start.
  reset();

  std::ifstream inputFile (filename, std::ios::in | std::ios::binary);

  // Assume no errors while disassembling.
  bool result = true;

  if (inputFile.is_open())
  {

    // Check if the input file is empty.
    // If so, bail out with an error.

    if (inputFile.peek() == std::ifstream::traits_type::eof())
    {
      error("File '" + filename + "' is empty!");
      return false;
    }


    // Read the instructions one at a time.
    qisa_instruction_type inst;

    // Used to keep track of the current instruction within the input file.
    size_t disassemblyInstructionCounter = 0;

    while(inputFile.read((char*)&inst, sizeof(qisa_instruction_type)))
    {
      if (_verbose)
      {
        std::bitset<sizeof(qisa_instruction_type)*8> binary(inst);
        std::cout << "Input instruction: " << getHex(inst, 8)
                  << " (" << binary << ")" << std::endl;
      }

      DisassembledInstruction disassembledInstruction;
      disassembledInstruction.address = disassemblyInstructionCounter;
      disassembledInstruction.hexCode = getHex(inst, 8);
      if (!disassembleInstruction(inst, disassembledInstruction))
      {
        _errorStream << "Error while disassembling instruction "
                     << getHex(inst, 8)
                     <<  ", instructionCount = " << disassemblyInstructionCounter;
        _errorLoc = location();
        result = false;
      }

      _disassembledInstructions[disassemblyInstructionCounter] = disassembledInstruction;

      disassemblyInstructionCounter++;
    }
    postProcessDisassembly();
  }
  else
  {
    error("Cannot open file '" + filename + "'.");
    return false;
  }

  // This is for save() to know it has to save disassembly output.
  _lastDriverAction = DRIVER_ACTION_DISASSEMBLE;
  return result;
}

void
QISA_Driver::postProcessDisassembly()
{
  if (_verbose)
    std::cout << "DISASSEMBLY POST-PROCESS" << std::endl;

  // This will be set to the actual label length in case branch
  // instructions are used.
  _disassemblyLabelStringLength = 0;

  // Only do the following in case labels have been used.
  if (!_disassemblyLabels.empty())
  {
    // Create a map from the used branch destination to label with an index.

    std::map<uint64_t, std::string> dest2LabelMap;
    int labelCounter = 0;

    // Calculate the number of digits needed to print the labels.
    // Source: https://stackoverflow.com/a/1489861
    int nrOfDigitsPerLabel = 0;

    size_t nrOfLabels = _disassemblyLabels.size();
    while (nrOfLabels != 0)
    {
      nrOfLabels /= 10;
      nrOfDigitsPerLabel++;
    }

    // Used to get the correct indentation in case there is no label.
    // The extra spaces (+ 2) are for the ": " that come after a 'full' label.
    _disassemblyLabelStringLength = strlen(DISASSEMBLY_LABEL_PREFIX) + nrOfDigitsPerLabel + 2;
    std::string emptyLabel(_disassemblyLabelStringLength, ' ');

    // Determine the name of the labels.
    for (auto it = _disassemblyLabels.begin(); it != _disassemblyLabels.end(); ++it)
    {
      std::ostringstream ssLabel;

      ssLabel << DISASSEMBLY_LABEL_PREFIX << std::setw(nrOfDigitsPerLabel) << std::setfill('0') << labelCounter;
      dest2LabelMap[it->first] = ssLabel.str();
      labelCounter++;
    }

    // Create a map that maps a branch instruction to its branch destination.
    std::map<uint64_t, uint64_t> branchInstr2destAddrMap;

    for (auto it = _disassemblyLabels.begin(); it != _disassemblyLabels.end(); ++it)
    {
      for (auto branchIt = it->second.begin(); branchIt != it->second.end(); ++branchIt)
      {
        branchInstr2destAddrMap[*branchIt] = it->first;
      }
    }

    if (_verbose)
      std::cout << "Processing " << branchInstr2destAddrMap.size() << " branch instructions..." << std::endl;

    for (auto itInstr = _disassembledInstructions.begin(); itInstr != _disassembledInstructions.end(); ++itInstr)
    {
      // If this is a branch destination, prepend the label instruction;
      auto itDest = _disassemblyLabels.find(itInstr->second.address);
      if (itDest != _disassemblyLabels.end())
      {
        itInstr->second.label = dest2LabelMap[itDest->first] + ": ";
      }
      else
      {
        // Else, just use spaces instead.
        itInstr->second.label = emptyLabel;
      }

      auto itBranchInstr = branchInstr2destAddrMap.find(itInstr->second.address);
      // If this is a branch instruction, emit its corresponding label and offset as comment.
      if (itBranchInstr != branchInstr2destAddrMap.end())
      {
        const int64_t offset = itBranchInstr->second - itInstr->second.address;

        std::ostringstream ssNewInstruction;
        ssNewInstruction << itInstr->second.instruction << ", " << dest2LabelMap[itBranchInstr->second]
                  << " # offset(" << std::showpos << offset << std::noshowpos << ")";
        itInstr->second.instruction = ssNewInstruction.str();
      }
    }
  }
}

std::string
QISA_Driver::getHex(uint64_t val, int nDigits)
{
  std::ostringstream ss;
  ss << "0x" << std::hex << std::setfill('0') << std::setw(nDigits) << val;
  return ss.str();
}

bool
QISA_Driver::disassembleInstruction(qisa_instruction_type inst, DisassembledInstruction& disassembledInst)
{
  if (inst & (1L << DBL_INST_FORMAT_BIT_OFFSET))
  {
    return disassembleQuantumInstruction(inst, disassembledInst);
  }
  else
  {
    // A quantum bundle ends when a non-bundle instruction is encountered.
    if (_disassemblyStartedQuantumBundle)
    {
      _disassemblyStartedQuantumBundle = false;
    }

    return disassembleClassicInstruction(inst, disassembledInst);
  }
}

bool
QISA_Driver::disassembleClassicInstruction(qisa_instruction_type inst, DisassembledInstruction& disassembledInst)
{
  int opc= (inst >> OPCODE_OFFSET) & OPCODE_MASK;

  // We don't deal with source code here, but want to use the checking
  // functions that are also used within the parser.
  // Define an empty location that we will use with these checking functions.
  location errLoc = location();

  if (_classicOpcode2instName.find(opc) == _classicOpcode2instName.end())
  {
    _errorStream << "Unknown opcode: " << getHex(opc, 2);
    _errorLoc = errLoc;
    return false;
  }

  std::string inst_name = _classicOpcode2instName[opc];

  // Might use a hashing function to speed things up if necessary.
  // Possible implementation: http://www.cse.yorku.ca/~oz/hash.html
  //
  // Djb2:
  //
  //  unsigned long
  //  hash(unsigned char *str)
  //  {
  //      unsigned long hash = 5381;
  //      int c;

  //      while (c = *str++)
  //          hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  //      return hash;
  //  }

  std::ostringstream ssInst;

  if ((inst_name == "NOP") ||
      (inst_name == "STOP"))
  {
    ssInst << inst_name;
  }
  else if ((inst_name == "ADD")  ||
           (inst_name == "ADDC") ||
           (inst_name == "SUB")  ||
           (inst_name == "SUBC") ||
           (inst_name == "AND")  ||
           (inst_name == "OR")   ||
           (inst_name == "XOR"))
  {
    const int rd = (inst >> RD_OFFSET) & RD_MASK;
    if (!checkRegisterNumber(rd, errLoc, R_REGISTER)) return false;

    const int rs = (inst >> RS_OFFSET) & RS_MASK;
    if (!checkRegisterNumber(rs, errLoc, R_REGISTER)) return false;

    const int rt = (inst >> RT_OFFSET) & RT_MASK;
    if (!checkRegisterNumber(rt, errLoc, R_REGISTER)) return false;

    ssInst << inst_name << " R" << rd << ", R" << rs << ", R" << rt;
  }
  else if (inst_name == "NOT")
  {
    const int rd = (inst >> RD_OFFSET) & RD_MASK;
    if (!checkRegisterNumber(rd, errLoc, R_REGISTER)) return false;

    const int rt = (inst >> RT_OFFSET) & RT_MASK;
    if (!checkRegisterNumber(rt, errLoc, R_REGISTER)) return false;

    ssInst << inst_name << " R" << rd << ", R" << rt;
  }
  else if (inst_name == "CMP")
  {
    const int rs = (inst >> RS_OFFSET) & RS_MASK;
    if (!checkRegisterNumber(rs, errLoc, R_REGISTER)) return false;

    const int rt = (inst >> RT_OFFSET) & RT_MASK;
    if (!checkRegisterNumber(rt, errLoc, R_REGISTER)) return false;

    ssInst << inst_name << " R" << rs << ", R" << rt;
  }
  else if (inst_name == "BR")
  {
    const int cond = inst & COND_MASK;

    if (_branchConditionNames.find(cond) == _branchConditionNames.end())
    {
      _errorStream << "Unknown branch condition: " << getHex(cond, 2);
      _errorLoc = location();
      return false;
    }

    const int addr = (inst >> ADDR_OFFSET) & ADDR_MASK;

    // Sign extend the address, which is an offset relative to the current instruction counter.
    // Source: http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
    struct {signed int x:21;} s;
    const int addr_offset = s.x = addr;

    const uint64_t dest_address = disassembledInst.address + addr_offset;

    // Mark the fact that this instruction is a branch instruction
    // that will need to address a label.
    _disassemblyLabels[dest_address].push_back(disassembledInst.address);

    // The labels will be added afterwards.
    ssInst << inst_name << " " << _branchConditionNames[cond];
  }
  else if (inst_name == "LDI")
  {
    const int rd = (inst >> RD_OFFSET) & RD_MASK;
    if (!checkRegisterNumber(rd, errLoc, R_REGISTER)) return false;

    const int imm = inst & IMM20_MASK;

    // Sign extend the immediate value.
    struct {signed int x:20;} s;
    const int signed_imm= s.x = imm;

    ssInst << inst_name << " R" << rd << ", "
           << getHex(signed_imm, 5) << " # dec("
           << signed_imm << ")";
  }
  else if (inst_name == "LDUI")
  {
    const int rd = (inst >> RD_OFFSET) & RD_MASK;
    if (!checkRegisterNumber(rd, errLoc, R_REGISTER)) return false;

    const int imm = inst & U_IMM15_MASK;

    ssInst << inst_name << " R" << rd << ", "
           << getHex(imm, 4) << " # dec("
           << imm << ")";
  }
  else if (inst_name == "FBR")
  {
    const int cond = inst & COND_MASK;

    if (_branchConditionNames.find(cond) == _branchConditionNames.end())
    {
      _errorStream << "Unknown branch condition: " << getHex(cond, 2);
      _errorLoc = location();
      return false;
    }

    const int rd = (inst >> RD_OFFSET) & RD_MASK;

    ssInst << inst_name << " " << _branchConditionNames[cond] << ", R" << rd;
  }
  else if (inst_name == "FMR")
  {
    const int rd = (inst >> RD_OFFSET) & RD_MASK;
    if (!checkRegisterNumber(rd, errLoc, R_REGISTER)) return false;

    const int qs = inst & QS_MASK;
    if (!checkRegisterNumber(qs, errLoc, Q_REGISTER)) return false;

    ssInst << inst_name << " R" << rd << ", Q" << qs;
  }
  else if (inst_name == "SMIS")
  {
    const int sd = (inst >> SD_OFFSET) & SD_MASK;
    if (!checkRegisterNumber(sd, errLoc, S_REGISTER)) return false;

    uint64_t s_mask_bits = (inst & S_MASK_MASK);

    auto s_mask = bits2s_mask(s_mask_bits);
    ssInst << inst_name << " S" << sd << ", " << get_s_mask_str(s_mask);
  }
  else if (inst_name == "SMIT")
  {
    const int td = (inst >> TD_OFFSET) & TD_MASK;
    if (!checkRegisterNumber(td, errLoc, T_REGISTER)) return false;

    uint64_t t_mask_bits = (inst & T_MASK_MASK);

    auto t_mask = bits2t_mask(t_mask_bits);
    ssInst << inst_name << " T" << td << ", " << get_t_mask_str(t_mask);
  }
  else if (inst_name == "QWAIT")
  {
    const int rd = (inst >> RD_OFFSET) & RD_MASK;
    if (!checkRegisterNumber(rd, errLoc, R_REGISTER)) return false;

    const int u_imm = inst & U_IMM20_MASK;

    ssInst << inst_name << " " << u_imm;
  }
  else if (inst_name == "QWAITR")
  {
    const int rs = (inst >> RS_OFFSET) & RS_MASK;
    if (!checkRegisterNumber(rs, errLoc, R_REGISTER)) return false;

    ssInst << inst_name << " R" << rs;
  }
  else
  {
    ssInst << "<Not yet supported: '"
           << _classicOpcode2instName[opc] << "'>" << std::endl;
  }

  disassembledInst.instruction = ssInst.str();

  return true;
}

bool
QISA_Driver::disassembleQuantumInstruction(qisa_instruction_type inst, DisassembledInstruction& disassembledInst)
{
  int bs = inst & BS_MASK;

  std::ostringstream ssInst;

  ssInst << "BS " << bs << " ";

#if 0 // Hold on to this for the high-level disassembly, if we will handle that.
  if (bs > 0)
  {
    // If a previous bundle was started, we have to issue a new-line to
    // terminate before starting a new one.
    if (_disassemblyStartedQuantumBundle)
    {
      _disassemblyOutput << std::endl;
    }
    // Now we start a new quantum bundle.
    _disassemblyOutput << "BS " << bs << " ";

    _disassemblyStartedQuantumBundle = true;
  }
  else
  {
    if (_disassemblyStartedQuantumBundle)
    {
      // If we already started a bundle, add this one to the set.
      _disassemblyOutput << " | ";
    }
    else
    {
      // Otherwise we start a new quantum bundle with 0 as wait time.
      _disassemblyOutput << "BS " << bs << " ";

      _disassemblyStartedQuantumBundle = true;
    }
  }
#endif

  // Quantum instructions are put pair-wise in a
  // 'very large instruction word' (VLIW).

  // Handle the first quantum instruction of the VLIW.
  // Last parameter contains the disassembled instruction.
  std::string q_0_str;
  if (!decode_q_instr((inst >> VLIW_INST_0_OFFSET) & VLIW_Q_INST_MASK, q_0_str))
  {
    // Return false if the binary value could not be decoded as a valid quantum instruction.
    return false;
  }

  std::string q_1_str;
  // Handle the second quantum instruction of the VLIW.
  if (!decode_q_instr((inst >> VLIW_INST_1_OFFSET) & VLIW_Q_INST_MASK, q_1_str))
  {
    // Return false if the binary value could not be decoded as a valid quantum instruction.
    return false;
  }

  // QNOPs are mostly hidden, unless both instructions of a VLIW are QNOPs.
  // If both instructions are QNOPs, issue one QNOP in total.
  // If only one is a QNOP, issue the non-QNOP instruction.
  // If neither instructions are QNOP, issue both instructions separated by a vertical bar.

  const bool q_0_is_nop = (q_0_str == "QNOP");
  const bool q_1_is_nop = (q_1_str == "QNOP");

  if (q_0_is_nop &&
      q_1_is_nop)
  {
    ssInst << q_0_str;
  }
  else if (q_0_is_nop)
  {
    ssInst << q_1_str;
  }
  else if (q_1_is_nop)
  {
    ssInst << q_0_str;
  }
  else
  {
    ssInst << q_0_str << " | " << q_1_str;
  }

  disassembledInst.instruction = ssInst.str();

  return true;
}


std::string
QISA_Driver::getLastErrorMessage()
{
  std::ostringstream ss;
  // Start with a new-line, so all lines will be indented properly.
  ss << std::endl;
  ss << getErrorSourceLine();
  ss << std::endl;
  ss << _errorStream.str();
  ss << std::endl;

  return ss.str();
}

// Lookup the error location in the source file and return its contents.
std::string
QISA_Driver::getErrorSourceLine()
{
  unsigned int start_error_line = _errorLoc.begin.line;
  unsigned int end_error_line = _errorLoc.end.line;
  unsigned int start_error_column = _errorLoc.begin.column;
  unsigned int end_error_column = _errorLoc.end.column;

  if ((start_error_line == 1) &&
      (end_error_line == 1) &&
      (start_error_column == 1) &&
      (end_error_column == 1))
  {
    // The location has not been initialized.
    // Return an empty string.
    return "";
  }

  bool error_was_on_prev_line = false;
  if (_errorStream.str().find("syntax error, unexpected NEWLINE") != std::string::npos)
  {
    error_was_on_prev_line = true;
    if ((start_error_line > 1) && (end_error_line > 1))
    {
      if (start_error_line == end_error_line)
      {
        end_error_line--;
      }
      start_error_line--;
    }
  }

  // Add context lines
  int start_context_line = start_error_line - NUM_CONTEXT_LINES_IN_ERROR_MSG;
  int end_context_line = end_error_line + NUM_CONTEXT_LINES_IN_ERROR_MSG;

  // Adjust start_context_line if it is too small.
  if (start_context_line < 1)
    start_context_line = 1;

  // Note that end_context_line may extend past the end of the file...

  std::ostringstream ss;

  std::ostringstream ss_pre_context;
  std::ostringstream ss_error_lines;
  std::ostringstream ss_marker_line;
  std::ostringstream ss_post_context;

  std::string line;
  std::string last_error_source_line;
  size_t line_counter = 0;

  std::ifstream srcFile (_filename);
  if (srcFile.is_open())
  {
    while ( std::getline (srcFile,line) )
    {
      line_counter++;

      if ((line_counter >= (size_t)start_context_line) &&
          (line_counter <= (size_t)end_context_line))
      {
        if (line_counter < start_error_line)
        {
          ss_pre_context << std::setfill(' ') << std::setw(8) << line_counter << ": ";
          ss_pre_context << line << std::endl;
        }
        else
          if ((line_counter >= start_error_line) &&
              (line_counter <= end_error_line))
          {
            last_error_source_line = line;
            ss_error_lines << std::setfill(' ') << std::setw(8) << line_counter << ": ";
            ss_error_lines << line << std::endl;
          }
          else
          {
            ss_post_context << std::setfill(' ') << std::setw(8) << line_counter << ": ";
            ss_post_context << line << std::endl;
          }
      }
      if (line_counter > (size_t)end_context_line)
      {
        break;
      }
    }
    srcFile.close();


    // Insert a set of carets (^) to show the exact location of the error.

    // Special case for when the error was an unexpected NEWLINE.
    // In that case, we want the error marker (^) to point to the
    // last valid position on the previous line.
    if (error_was_on_prev_line)
    {
      start_error_column = last_error_source_line.size() + 1;

      // Check if there is a comment character (#) in this line.
      // Discard everything after that character.
      size_t comment_pos = last_error_source_line.find("#");
      if (comment_pos != std::string::npos)
      {
        start_error_column = comment_pos + 1;
      }

      end_error_column = start_error_column + 1;
    }

    for (unsigned int i = 1; i < end_error_column; i++)
    {
      if (i < start_error_column)
      {
        if (last_error_source_line[i-1] == '\t')
        {
          ss_marker_line << "\t";
        }
        else
        {
          ss_marker_line << " ";
        }
      }
      else
      {
        ss_marker_line << "^";
      }
    }
    ss_marker_line << std::endl;

    ss << ss_pre_context.str();
    ss << "-------------------------------" << std::endl;
    ss << ss_error_lines.str();
    // Note: add 10 spaces to account for the line number.
    ss << "          " << ss_marker_line.str();
    ss << "-------------------------------" << std::endl;
    ss << ss_post_context.str();

  }
  else
  {
    ss << "<Could not read from file: " << _filename << ">";
  }

  return ss.str();
}

std::string
QISA_Driver::getVersion()
{
  return QISA_VERSION_STRING;
}

std::string
QISA_Driver::dumpInstructionsSpecification()
{
  if (_opcodes.empty())
  {
    setOpcodes();
  }

  std::ostringstream ss;
  ss << "##################################################" << std::endl;
  ss << "#                                                #" << std::endl;
  ss << "#            QISA Opcode Specification           #" << std::endl;
  ss << "#                                                #" << std::endl;
  ss << "# Compilation-date: " << __DATE__ << "                  #" << std::endl;
  ss << "# Compilation-time: " << __TIME__ << "                     #" << std::endl;
  ss << "#                                                #" << std::endl;
  ss << "##################################################" << std::endl;
  ss << std::endl;
  ss << "#" << std::endl;
  ss << "# Compiled-in classic instructions" << std::endl;
  ss << "# (Single Instruction Format)" << std::endl;
  ss << "#" << std::endl;
  ss << std::endl;

  std::string opc_str;

  for (auto it : _opcodes)
  {
    opc_str = "def_opcode['" + it.first+ "']";
    ss << std::setw(30) << std::left << opc_str << "= " << getHex(it.second, 2) << std::endl;
  }

  ss << std::endl;
  ss << "#" << std::endl;
  ss << "# Configured quantum instructions" << std::endl;
  ss << "# (Double Instruction Format)" << std::endl;
  ss << "#" << std::endl;

  ss << std::endl;
  ss << "# No arguments" << std::endl;
  ss << "# (def_q_arg_none)" << std::endl;

  for (auto it : _q_inst_arg_none_opcodes)
  {
    opc_str = "def_q_arg_none['" + it.first + "']";
    ss << std::setw(30) << std::left << opc_str << "= " << getHex(it.second, 2) << std::endl;
  }

  ss << std::endl;
  ss << "# Uses 'S' register as parameter" << std::endl;
  ss << "# (def_q_arg_st)" << std::endl;

  for (auto it : _q_inst_arg_st_opcodes)
  {
    opc_str = "def_q_arg_st['" + it.first + "']";
    ss << std::setw(30) << std::left << opc_str << "= " << getHex(it.second, 2) << std::endl;
  }

  ss << std::endl;
  ss << "# Uses 'T' register as parameter" << std::endl;
  ss << "# (def_q_arg_tt)" << std::endl;

  for (auto it : _q_inst_arg_tt_opcodes)
  {
    opc_str = "def_q_arg_tt['" + it.first + "']";
    ss << std::setw(30) << std::left << opc_str << "= " << getHex(it.second, 2) << std::endl;
  }

  return ss.str();
}



void
QISA_Driver::enableScannerTracing(bool enabled)
{
  _traceScanning = enabled;
}

void
QISA_Driver::enableParserTracing(bool enabled)
{
  _traceParsing = enabled;
}

void
QISA_Driver::setVerbose(bool verbose)
{
  _verbose = verbose;
}

void
QISA_Driver::error(const location& l, const std::string& m)
{
  _errorStream << _filename << ":" << l << ": " << m << std::endl;
  _errorLoc = l;
}

void
QISA_Driver::error(const std::string& m)
{
  _errorStream << m << std::endl;
  _errorLoc = location();
}

void
QISA_Driver::addSpecificErrorMessage(const std::string& msg)
{
  _errorStream << msg << std::endl;
}

void
QISA_Driver::addExpectationErrorMessage(const std::string& expected_item)
{
  if (_errorStream.str().find(", expecting") == std::string::npos)
  {
    addSpecificErrorMessage("ERROR DETECTED: Expected " + expected_item + " here");
  }
}

void
QISA_Driver::addExpectedConditionErrorMessage()
{
  if (_errorStream.str().find(", expecting") == std::string::npos)
  {
    std::ostringstream ss;
    ss << std::endl;
    ss << "ERROR DETECTED: Expected a CONDITION here." << std::endl;
    ss << "Valid conditions are:" << std::endl;
    for (auto it = _branchConditionNames.begin(); it != _branchConditionNames.end(); ++it)
    {
      ss << "  " << it->second << std::endl;
    }
    ss << std::endl;
    ss << "Valid condition ALIASES are:" << std::endl;
    for (auto it = _branchConditionAliases.begin(); it != _branchConditionAliases.end(); ++it)
    {
      ss << "  " << std::setw(9) << std::left << it->first << ": aliased to: " << _branchConditionNames[it->second] << std::endl;
    }

    addSpecificErrorMessage(ss.str());
  }
}


void
QISA_Driver::add_symbol(const std::string& symbol_name,
                        const QISA::location& symbol_name_loc,
                        int64_t symbol_value,
                        const QISA::location& symbol_value_loc)
{
  if (_verbose)
      std::cout <<  "          "
                << "ADD_SYMBOL[int](name='" << symbol_name << "', val=" << symbol_value << ");" << std::endl;

  // Note that if a symbol already exists, its value will be overwritten.
  _intSymbols[symbol_name] = symbol_value;

}

bool
QISA_Driver::get_symbol(const std::string& symbol_name,
                        const QISA::location& symbol_name_loc,
                        int64_t& imm_val)
{
  if (_verbose)
      std::cout <<  "          "
                << "GET_SYMBOL[int](name='" << symbol_name << "');" << std::endl;

  auto findIt = _intSymbols.find(symbol_name);

  if (findIt == _intSymbols.end())
  {
    _errorStream << symbol_name_loc << ": symbol '" << symbol_name << "' not found" << std::endl;
    _errorLoc = symbol_name_loc;
    return false;
  }

  imm_val = findIt->second;
  return true;
}

// Add a string valued symbol (not used yet).
void
QISA_Driver::add_symbol(const std::string& symbol_name,
                        const QISA::location& symbol_name_loc,
                        const std::string&  symbol_value,
                        const QISA::location& symbol_value_loc)
{
  if (_verbose)
      std::cout <<  "          "
                << "ADD_SYMBOL[str](name='" << symbol_name << "', val=" << symbol_value << ");" << std::endl;
  _strSymbols[symbol_name] = symbol_value;
}


bool
QISA_Driver::get_symbol(const std::string& symbol_name,
                        const QISA::location& symbol_name_loc,
                        std::string& imm_val)
{
  if (_verbose)
      std::cout <<  "          "
                << "GET_SYMBOL[str](name='" << symbol_name << "');" << std::endl;

  auto findIt = _strSymbols.find(symbol_name);

  if (findIt == _strSymbols.end())
  {
    _errorStream << symbol_name_loc << ": symbol '" << symbol_name << "' not found" << std::endl;
    _errorLoc = symbol_name_loc;
    return false;
  }

  imm_val = findIt->second;
  return true;
}

// Add a register definition.
// This is used to give a register a meaningful name.
bool
QISA_Driver::add_register_definition(const std::string& register_name,
                                     const QISA::location& register_name_loc,
                                     uint8_t reg_nr,
                                     const QISA::location& reg_nr_loc,
                                     RegisterKind register_kind)
{
  if (_verbose)
      std::cout << "          "
                << "DEFINE_REG(name='" << register_name
                << "', reg=" << _registerName[register_kind] << (int)reg_nr << ");" << std::endl;

  if (!checkRegisterNumber(reg_nr, reg_nr_loc, register_kind))
  {
    return false;
  }

  _registerAliases[register_kind][register_name] = reg_nr;
  return true;

}

bool
QISA_Driver::get_register_nr(const std::string& register_name,
                             const QISA::location& register_name_loc,
                             RegisterKind register_kind,
                             uint8_t& result)
{
  if (_verbose)
      std::cout <<  "          "
                << "GET_REG(name='" << register_name << "');" << std::endl;

  auto findIt = _registerAliases[register_kind].find(register_name);

  if (findIt == _registerAliases[register_kind].end())
  {
    _errorStream << register_name_loc << ": '"
                 << _registerName[register_kind]
                 << "' register named '" << register_name
                 << "' not found" << std::endl;
    _errorLoc = register_name_loc;
    return false;
  }

  result = findIt->second;
  return true;
}

void
QISA_Driver::add_label(const std::string& label_name,
                       const QISA::location& label_name_loc)
{
  if (_verbose)
      std::cout <<  "          "
                << "ADD_LABEL(name='" << label_name << "') -> addr=" << _instructions.size() << ";" << std::endl;
  _labels[label_name] = _instructions.size();
}

int64_t
QISA_Driver::get_label_address(const std::string& label_name,
                               const QISA::location& label_name_loc,
                               bool get_offset)
{
  if (_verbose)
      std::cout <<  "          "
                << "GET_LABEL_ADDRESS(name='" << label_name << "');" << std::endl;


  const uint64_t programCounter = _instructions.size();

  int64_t result;

  auto findIt = _labels.find(label_name);

  if (findIt == _labels.end())
  {
    // This label has not yet been defined.
    // Record all information that is necessary to assemble the instruction that uses this label after the
    // whole source file has been processed.

    DeferredLabelUse deferred;

    // The instruction type (token) and programCounter will be set when the instruction
    // on the current line will be handled.
    deferred.is_offset = get_offset;
    deferred.label_name = label_name;
    deferred.label_name_loc = label_name_loc;

    // Check if a label has already been used in the current instruction.
    auto itDeferred = _deferredInstructions.find(label_name_loc);

    if (itDeferred == _deferredInstructions.end())
    {
      // If not, add this deferred label use to the deferred instruction map.

      _deferredInstructions[label_name_loc] = std::move(deferred);
    }
    else
    {
      // Another label has already been used in this instruction.
      // To indicate this error, modify the original label name and set the location to the offending label.
      itDeferred->second.label_name = "<<MULTI_LABEL_ERROR>>";
      itDeferred->second.label_name_loc = label_name_loc;
    }

    // This should be a non-sensical return value...
    result = std::numeric_limits<int64_t>::min();

    if (_verbose)
        std::cout << "          "
                  << "    GET_LABEL_ADDRESS didn't find label '" << label_name << "' (yet)" << std::endl;
  }
  else
  {
    if (_verbose)
        std::cout << "          "
                  << "    GET_LABEL_ADDRESS found label: '" << findIt->first << "', address=: " << findIt->second << std::endl;
    if (get_offset)
    {
      result = findIt->second - programCounter;
    }
    else
    {
      result = findIt->second;
    }
  }

  return result;
}

bool
QISA_Driver::get_opcode(const std::string& instruction_name,
                        const QISA::location& instruction_name_loc,
                        int& opcode,
                        QISA_InstructionKind instruction_kind)
{
  const std::map<std::string, int>* opCodeMap;

  switch (instruction_kind) {
  case IK_SINGLE_FORMAT:
    opCodeMap = &_opcodes;
    break;
  case IK_DF_ARG_NONE:
    opCodeMap = &_q_inst_arg_none_opcodes;
    break;
  case IK_DF_ARG_ST:
    opCodeMap = &_q_inst_arg_st_opcodes;
    break;
  case IK_DF_ARG_TT:
    opCodeMap = &_q_inst_arg_tt_opcodes;
    break;
  }

  std::string searchString(instruction_name);

  // Make the instruction name uppercase.
  // https://stackoverflow.com/a/17793588
  for (auto & c: searchString) c = toupper((unsigned char)c);

  auto findIt = opCodeMap->find(searchString);

  if (findIt == opCodeMap->end())
  {
    // The opcode of the requested instruction kind has not been found.
    // Check if it is specified in one of the other opcode maps.
    // If so, the user has mixed up his parameters, so give him a hint about that.

    if ((instruction_kind != IK_SINGLE_FORMAT) &&
        _opcodes.find(searchString) != _opcodes.end())
    {
      _errorStream << instruction_name_loc << ": "
                   << "Classic instruction '" << instruction_name
                   << "' used instead of a quantum instruction. "
                   << std::endl;
    }
    else if ((instruction_kind != IK_DF_ARG_NONE) &&
        _q_inst_arg_none_opcodes.find(searchString) != _q_inst_arg_none_opcodes.end())
    {
      _errorStream << instruction_name_loc << ": "
                   << "Instruction '" << instruction_name
                   << "' takes no parameters."
                   << std::endl;

    }
    else if ((instruction_kind != IK_DF_ARG_ST) &&
        _q_inst_arg_st_opcodes.find(searchString) != _q_inst_arg_st_opcodes.end())
    {
      _errorStream << instruction_name_loc << ": "
                   << "Instruction '" << instruction_name
                   << "' needs an S-register argument."
                   << std::endl;
    }
    else if ((instruction_kind != IK_DF_ARG_TT) &&
        _q_inst_arg_tt_opcodes.find(searchString) != _q_inst_arg_tt_opcodes.end())
    {
      _errorStream << instruction_name_loc << ": "
                   << "Instruction '" << instruction_name
                   << "' needs a T-register argument."
                   << std::endl;
    }
    else
    {
      _errorStream << instruction_name_loc << ": opcode for '" << instruction_name << "' not found" << std::endl;
    }

    _errorLoc = instruction_name_loc;
    return false;
  }

  opcode = findIt->second;
  return true;
}

// Assembly generation functions.

/* nop */
bool
QISA_Driver::generate_NOP(const QISA::location& inst_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "NOP();" << std::endl;
  int opcode;

  if (!get_opcode("NOP", inst_loc, opcode))
  {
    return false;
  }

  qisa_instruction_type instruction = (((opcode & OPCODE_MASK) & OPCODE_MASK) << OPCODE_OFFSET);

  _instructions.emplace_back(instruction);
  return true;
}


/* stop */
bool
QISA_Driver::generate_STOP(const QISA::location& inst_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "STOP();" << std::endl;
  int opcode;

  if (!get_opcode("STOP", inst_loc, opcode))
  {
    return false;
  }

  qisa_instruction_type instruction = (((opcode & OPCODE_MASK) & OPCODE_MASK) << OPCODE_OFFSET);

  _instructions.emplace_back(instruction);
  return true;
}

/* add rd, rs, rt */
/* sub rd, rs, rt */
/* and rd, rs, rt */
/* or  rd, rs, rt */
/* xor rd, rs, rt */
bool
QISA_Driver::generate_XXX_rd_rs_rt(const std::string& inst_name,
                                   const QISA::location& inst_loc,
                                   uint8_t rd,
                                   const QISA::location& rd_loc,
                                   uint8_t rs,
                                   const QISA::location& rs_loc,
                                   uint8_t rt,
                                   const QISA::location& rt_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << inst_name << "(rd=" << (int)rd << ",rs=" << (int)rs << ",rt=" << (int)rt << ");" << std::endl;

  int opcode;

  if (!get_opcode(inst_name, inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rd, rd_loc, R_REGISTER))
  {
    return false;
  }
  if (!checkRegisterNumber(rs, rs_loc, R_REGISTER))
  {
    return false;
  }
  if (!checkRegisterNumber(rt, rt_loc, R_REGISTER))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rd & RD_MASK) << RD_OFFSET)
                                      | ((rs & RS_MASK) << RS_OFFSET)
                                      | ((rt & RT_MASK) << RT_OFFSET);

  _instructions.emplace_back(instruction);
  return true;
}

/* not rd, rt */
bool QISA_Driver::generate_NOT(const location& inst_loc,
                               uint8_t rd,
                               const location& rd_loc,
                               uint8_t rt,
                               const location& rt_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "NOT(rd=" << (int)rd << ",rt=" << (int)rt << ");" << std::endl;

  int opcode;

  if (!get_opcode("NOT", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rd, rd_loc, R_REGISTER))
  {
    return false;
  }
  if (!checkRegisterNumber(rt, rt_loc, R_REGISTER))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rd & RD_MASK) << RD_OFFSET)
                                      | ((rt & RT_MASK) << RT_OFFSET);

  _instructions.emplace_back(instruction);
  return true;

}

/* cmp rs,rt */
bool
QISA_Driver::generate_CMP(const QISA::location& inst_loc,
                          uint8_t rs,
                          const QISA::location& rs_loc,
                          uint8_t rt,
                          const QISA::location& rt_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "CMP(rs=" << (int)rs << ",rt=" << (int)rt << ");" << std::endl;

  int opcode;

  if (!get_opcode("CMP", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rs, rs_loc, R_REGISTER))
  {
    return false;
  }
  if (!checkRegisterNumber(rt, rt_loc, R_REGISTER))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rs & RS_MASK) << RS_OFFSET)
                                      | ((rt & RT_MASK) << RT_OFFSET);

  _instructions.emplace_back(instruction);
  return true;
}

/* br cond, addr */
bool
QISA_Driver::generate_BR(const location& inst_loc,
                         uint8_t cond,
                         const location& cond_loc,
                         int64_t addr,
                         const location& addr_loc,
                         bool is_alias)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "BR(cond='" << _branchConditionNames[cond] << "',addr=" << addr << ");" << std::endl;

  int opcode;

  if (!get_opcode("BR", inst_loc, opcode))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | (cond & COND_MASK);


  // Handle the label part.
  // If the label is not yet defined, we will handle it later.
  if (addr != std::numeric_limits<int64_t>::min())
  {
    // The offset is based on the current program counter.

    // An offset is encoded using 21 bits (signed).
    // Check if the given value is within this range.

    const int64_t minOffset = -(1LL<<20) + 1;
    const int64_t maxOffset =  (1LL<<20) - 1;

    if (!checkValueRange(addr, minOffset, maxOffset, "addr", addr_loc))
    {
      return false;
    }

    if (is_alias)
    {
      // Correct for the implicit CMP instruction that precedes the actual branch instruction.
      if (addr < 0)
      {
        addr--;
      }
      else
      {
        // The positive case is already been handled implicitely by the fact that
        // the program counter already advanced by one due to the CMP instruction.
      }
    }

    instruction |= ((addr & ADDR_MASK) << ADDR_OFFSET);
  }
  else
  {
    // A deferred instruction has been created in the attempt to resolve a label.
    // Locate this and set the instruction type.

    auto findIt = _deferredInstructions.find(addr_loc);
    if (findIt == _deferredInstructions.end())
    {
      // This should not happen.
      _errorStream << "INTERNAL ASSEMBLER ERROR <BR:DEFER>, location=" << addr_loc << std::endl;
      _errorLoc = addr_loc;
      return false;
    }
    else
    {
      findIt->second.instruction = QISA_Parser::token::TOK_BR;
      findIt->second.programCounter = _instructions.size();
    }
  }

  _instructions.emplace_back(instruction);

  return true;
}

/* ldi rd, imm */
bool QISA_Driver::generate_LDI(const location& inst_loc,
                               uint8_t rd,
                               const location& rd_loc,
                               int64_t imm,
                               const location& imm_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "LDI(rd=" << (int)rd << ",imm=" << imm << ");" << std::endl;

  int opcode;

  if (!get_opcode("LDI", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rd, rd_loc, R_REGISTER))
  {
    return false;
  }

  // The 'imm' value is encoded using 20 bits (signed).
  // Check if the given value is within this range.

  const int64_t minImm = -(1LL<<19) + 1;
  const int64_t maxImm =  (1LL<<19) - 1;

  if (!checkValueRange(imm, minImm, maxImm, "imm", imm_loc))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rd & RD_MASK) << RD_OFFSET)
                                      | (imm & IMM20_MASK);

  _instructions.emplace_back(instruction);
  return true;
}

/* ldui rd, u_imm */
bool QISA_Driver::generate_LDUI(const location& inst_loc,
                               uint8_t rd,
                                const location& rd_loc,
                               int64_t imm,
                                const location& imm_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "LDUI(rd=" << (int)rd << ",imm=" << imm << ");" << std::endl;

  int opcode;

  if (!get_opcode("LDUI", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rd, rd_loc, R_REGISTER))
  {
    return false;
  }

  // The 'imm' value is encoded using 15 bits (unsigned).
  // Check if the given value is within this range.

  const int64_t minImm = 0;
  const int64_t maxImm = (1LL<<15) - 1;

  if (!checkValueRange(imm, minImm, maxImm, "imm", imm_loc))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rd & RD_MASK) << RD_OFFSET)
                                      | ((rd & RS_MASK) << RS_OFFSET) // Note: rs <-- rd
                                      | (imm & U_IMM15_MASK);

  _instructions.emplace_back(instruction);
  return true;
}

/* fbr cond, rd */
bool
QISA_Driver::generate_FBR(const location& inst_loc,
                          uint8_t cond,
                          const location& cond_loc,
                          uint8_t rd,
                          const location& rd_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "FBR(cond='" << _branchConditionNames[cond] << "',rd=" << (int)rd << ");" << std::endl;

  int opcode;

  if (!get_opcode("FBR", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rd, rd_loc, R_REGISTER))
  {
    return false;
  }


  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rd & RD_MASK) << RD_OFFSET)
                                      | (cond & COND_MASK);

  _instructions.emplace_back(instruction);

  return true;
}

/* fmr rd, qs */
bool
QISA_Driver::generate_FMR(const location& inst_loc,
                          uint8_t rd,
                          const location& rd_loc,
                          uint8_t qs,
                          const location& qs_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "FMR(rd=" << (int)rd << ",qs=" << (int)qs << ");" << std::endl;

  int opcode;

  if (!get_opcode("FMR", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rd, rd_loc, R_REGISTER))
  {
    return false;
  }

  if (!checkRegisterNumber(qs, qs_loc, Q_REGISTER))
  {
    return false;
  }


  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rd & RD_MASK) << RD_OFFSET)
                                      | (qs & QS_MASK);

  _instructions.emplace_back(instruction);

  return true;
}

/* smis sd, s_mask */
bool
QISA_Driver::generate_SMIS(const location& inst_loc,
                           uint8_t sd,
                           const location& sd_loc,
                           const std::vector<uint8_t>& s_mask,
                           const location& s_mask_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "SMIS(sd=" << (int)sd << ",s_mask=" << get_s_mask_str(s_mask) << ");" << std::endl;

  int opcode;

  if (!get_opcode("SMIS", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(sd, sd_loc, S_REGISTER))
  {
    return false;
  }

  // Construct the s_mask on bit level.
  // NOTE: The parser already has checked the given s_mask.
  int64_t s_mask_bits = 0;
  for (auto it = s_mask.begin(); it != s_mask.end(); ++it)
  {
    s_mask_bits |= (1LL << *it);
  }

  //divide it into two parts
  //int64_t s_mask_bits = s_mask_bits & 0x1ffff;
  int64_t s_mask_bits_array;
  s_mask_bits_array = s_mask_bits & 0x1ffff;

  //corresponding pos value
  //No position field for SMIS

  // Encode the parameters into an instruction and add it to the instruction list.
  unsafe_generate_SMIS(opcode, sd, s_mask_bits_array);
  //unsafe_generate_SMIS(opcode, sd, pos1, s_mask_bits1);
  //unsafe_generate_SMIS(opcode, sd, pos2, s_mask_bits2);

  return true;
}


std::vector<uint8_t>
QISA_Driver::bits2s_mask(int64_t s_mask_bits)
{
  std::vector<uint8_t> result;
  const std::bitset<sizeof(s_mask_bits)> bs(s_mask_bits);
  for (uint8_t i = 0; i < _totalNrOfQubits; i++)
  {
    if (bs[i])
    {
      result.push_back(i);
    }
  }

  return result;
}

std::vector<QISA_Driver::TargetControlPair>
QISA_Driver::bits2t_mask(int64_t t_mask_bits)
{
  std::vector<TargetControlPair>  result;
  for (size_t i = 0; i < _valid_target_control_pairs.size(); i++)
  {
    if (t_mask_bits & ( 1LL<< i))
    {
      result.push_back(_bit2tc_pair[i]);
    }
  }

  return result;
}


/* smis sd, imm  (NOTE: Alternative representation.) */
bool
QISA_Driver::generate_SMIS(const location& inst_loc,
                           uint8_t sd,
                           const location& sd_loc,
                           int64_t imm,
                           const location& imm_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "SMIS(sd=" << (int)sd << ",imm=" << imm << ");" << std::endl;

  int opcode;

  if (!get_opcode("SMIS", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(sd, sd_loc, S_REGISTER))
  {
    return false;
  }

  // The 'imm' value is encoded using 7 bits (unsigned).
  // Check if the given value is within this range.

  const int64_t minImm = 0;
  const int64_t maxImm = (1LL<<17) - 1;

  if (!checkValueRange(imm, minImm, maxImm, "imm", imm_loc))
  {
    return false;
  }

  // Encode the parameters into an instruction and add it to the instruction list.
  unsafe_generate_SMIS(opcode, sd, imm);

  return true;
}



// Used by the two flavors of generate_SMIS.
// All arguments are supposed to have been checked.
void
QISA_Driver::unsafe_generate_SMIS(int opcode,
                                  uint8_t sd,
                                  int64_t s_mask_bits)
{
  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((sd & SD_MASK) << SD_OFFSET)
                                      | (s_mask_bits & S_MASK_MASK);

  _instructions.emplace_back(instruction);
}


/* smit td, t_mask */
bool
QISA_Driver::generate_SMIT(const location& inst_loc,
                           uint8_t td,
                           const location& td_loc,
                           const std::vector<TargetControlPair>& t_mask,
                           const location& t_mask_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "SMIT(td=" << (int)td << ",t_mask=" << get_t_mask_str(t_mask) << ");" << std::endl;

  int opcode;

  if (!get_opcode("SMIT", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(td, td_loc, T_REGISTER))
  {
    return false;
  }

  // Construct the t_mask on bit level.
  // NOTE: The parser already has checked the given t_mask.

  int64_t t_mask_bits = 0;
  for (auto it = t_mask.begin(); it != t_mask.end(); ++it)
  {
    const uint8_t t_mask_bit = _valid_target_control_pairs[*it];
    t_mask_bits |= (1LL << t_mask_bit);
  }

  //divide the 48 bits mask value into three parts
  //int64_t t_mask_bits1 = (t_mask_bits & 0xffff);
  //int64_t t_mask_bits2 = (t_mask_bits & 0xffff0000) >> 16;
  //int64_t t_mask_bits3 = (t_mask_bits & 0xffff00000000) >> 32;
  int64_t *t_mask_bits_array;
  t_mask_bits_array = new int64_t[pos_number_t];
  for (int i = 0; i < pos_number_t; ++i) {
    t_mask_bits_array[i] = (t_mask_bits >> (16 * i)) & 0xffff;
  }

  //corresponding pos value
  //uint8_t pos1 = 0;
  //uint8_t pos2 = 1;
  //uint8_t pos3 = 2;
  uint8_t *pos;
  pos = new uint8_t[pos_number_t];
  for (int i = 0; i < pos_number_t; ++i) {
    pos[i] = i;
  }

  // Encode the parameters into an instruction and add it to the instruction list.
  //unsafe_generate_SMIT(opcode, td, pos1, t_mask_bits1);
  //unsafe_generate_SMIT(opcode, td, pos2, t_mask_bits2);
  //unsafe_generate_SMIT(opcode, td, pos3, t_mask_bits3);
  for (int i = 0; i < pos_number_t; ++i) {
    unsafe_generate_SMIT(opcode, td, pos[i], t_mask_bits_array[i]);
  }

  return true;
}

/* smit td, imm  (NOTE: Alternative representation.) */
bool
QISA_Driver::generate_SMIT(const location& inst_loc,
                           uint8_t td,
                           const location& td_loc,
                           int64_t imm,
                           const location& imm_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "SMIT(td=" << (int)td << ",imm=" << imm << ");" << std::endl;

  int opcode;

  if (!get_opcode("SMIT", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(td, td_loc, T_REGISTER))
  {
    return false;
  }

  // The 'imm' value is encoded using 16 bits (unsigned).
  // Check if the given value is within this range.

  const int64_t minImm = 0;
  const int64_t maxImm = (1LL<<48) - 1;

  if (!checkValueRange(imm, minImm, maxImm, "imm", imm_loc))
  {
    return false;
  }

  // Now, we must subject this value to the same tests as done in validate_t_mask.

  // First, we will convert the given immediate value to a vector of tc_pairs.
  auto t_mask = bits2t_mask(imm);

  // Now we can perform the check.
  if (!validate_t_mask(t_mask, imm_loc))
  {
    return false;
  }

  // Encode the parameters into an instruction and add it to the instruction list.
  unsafe_generate_SMIT(opcode, td, 0, imm);

  return true;
}



// Used by the two flavors of generate_SMIT.
// All arguments are supposed to have been checked.
void
QISA_Driver::unsafe_generate_SMIT(int opcode,
                                  uint8_t td,
                                  uint8_t pos,
                                  int64_t t_mask_bits)
{
  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((td & TD_MASK) << TD_OFFSET)
                                      | ((pos & POS_MASK) << POS_OFFSET)
                                      | (t_mask_bits & T_MASK_MASK);

  _instructions.emplace_back(instruction);
}


/* qwait u_imm */
bool
QISA_Driver::generate_QWAIT(const location& inst_loc,
                            int64_t imm,
                            const location& imm_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "QWAIT(u_imm=" << imm << ");" << std::endl;

  int opcode;

  if (!get_opcode("QWAIT", inst_loc, opcode))
  {
    return false;
  }

  // The 'imm' value is encoded using 20 bits (unsigned).
  // Check if the given value is within this range.

  const int64_t minImm = 0;
  const int64_t maxImm = (1LL<<20) - 1;

  if (!checkValueRange(imm, minImm, maxImm, "imm", imm_loc))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | (imm & U_IMM20_MASK);

  _instructions.emplace_back(instruction);
  return true;
}

/* qwaitr rs */
bool
QISA_Driver::generate_QWAITR(const QISA::location& inst_loc,
                             uint8_t rs,
                             const QISA::location& rs_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "QWAITR(rs=" << (int)rs << ");" << std::endl;

  int opcode;

  if (!get_opcode("QWAITR", inst_loc, opcode))
  {
    return false;
  }

  if (!checkRegisterNumber(rs, rs_loc, R_REGISTER))
  {
    return false;
  }

  qisa_instruction_type instruction = ((opcode & OPCODE_MASK) << OPCODE_OFFSET)
                                      | ((rs & RS_MASK) << RS_OFFSET);

  _instructions.emplace_back(instruction);
  return true;
}

/* ALIAS: [SHL1 rd,rs] --> [ADD rd,rs,rs] */
bool
QISA_Driver::generate_SHL1(const location& inst_loc,
                           uint8_t rd,
                           const location& rd_loc,
                           uint8_t rs,
                           const location& rs_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: SHL1(rd=" << (int)rd << ",rs=" << (int)rs << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.



  bool result = generate_XXX_rd_rs_rt("ADD", inst_loc, rd, rd_loc, rs, rs_loc, rs, rs_loc);
  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* ALIAS: [NAND rd,rs,rt] --> [AND rd,rs,rt ; NOT rd,rd] */
bool
QISA_Driver::generate_NAND(const location& inst_loc,
                           uint8_t rd,
                           const location& rd_loc,
                           uint8_t rs,
                           const location& rs_loc,
                           uint8_t rt,
                           const location& rt_loc)
{

  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: NAND(rd=" << (int)rd << ",rs=" << (int)rs << ",rt=" << (int)rt << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_XXX_rd_rs_rt("AND", inst_loc, rd, rd_loc, rs, rs_loc, rt, rt_loc);

  if (result)
  {
    result = generate_NOT(inst_loc, rd, rd_loc, rd, rd_loc);
  }

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* ALIAS: [NOR rd,rs,rt] --> [OR rd,rs,rt ; NOT rd,rd] */
bool
QISA_Driver::generate_NOR(const location& inst_loc,
                          uint8_t rd,
                          const location& rd_loc,
                          uint8_t rs,
                          const location& rs_loc,
                          uint8_t rt,
                          const location& rt_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: NOR(rd=" << (int)rd << ",rs=" << (int)rs << ",rt=" << (int)rt << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_XXX_rd_rs_rt("OR", inst_loc, rd, rd_loc, rs, rs_loc, rt, rt_loc);

  if (result)
  {
    result = generate_NOT(inst_loc, rd, rd_loc, rd, rd_loc);
  }

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* ALIAS: [XNOR rd,rs,rt] --> [XOR rd,rs,rt ; NOT rd,rd] */
bool
QISA_Driver::generate_XNOR(const location& inst_loc,
                           uint8_t rd,
                           const location& rd_loc,
                           uint8_t rs,
                           const location& rs_loc,
                           uint8_t rt,
                           const location& rt_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: XNOR(rd=" << (int)rd << ",rs=" << (int)rs << ",rt=" << (int)rt << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_XXX_rd_rs_rt("XOR", inst_loc, rd, rd_loc, rs, rs_loc, rt, rt_loc);

  if (result)
  {
    result = generate_NOT(inst_loc, rd, rd_loc, rd, rd_loc);
  }

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* ALIAS: [BRA addr] --> [BR always, addr] */
bool
QISA_Driver::generate_BRA(const location& inst_loc,
                          int64_t addr,
                          const location& addr_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: BRA(addr=" << addr << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_BR(inst_loc, COND_ALWAYS, inst_loc, addr, addr_loc);

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* ALIAS: [GOTO addr] --> [BR always, addr] */
bool
QISA_Driver::generate_GOTO(const location& inst_loc,
                           int64_t addr,
                           const location& addr_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: GOTO(addr=" << addr << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_BR(inst_loc, COND_ALWAYS, inst_loc, addr, addr_loc);

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}


/* ALIAS: [BRN addr] --> [BR never, addr] */
bool
QISA_Driver::generate_BRN(const location& inst_loc,
                          int64_t addr,
                          const location& addr_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: BRN(addr=" << addr << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_BR(inst_loc, COND_NEVER, inst_loc, addr, addr_loc);

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* BEQ   rs, rt, addr  */
/* BNE   rs, rt, addr  */
/* BLT   rs, rt, addr  */
/* BLE   rs, rt, addr  */
/* BGT   rs, rt, addr  */
/* BGE   rs, rt, addr  */
/* BLTU  rs, rt, addr  */
/* BLEU  rs, rt, addr  */
/* BGTU  rs, rt, addr  */
/* BGEU  rs, rt, addr  */

/* ALIAS: [BXX rs,rt,addr] --> [CMP rs,rt; BR XX,addr] */
bool
QISA_Driver::generate_BR_COND(const location& inst_loc,
                              uint8_t rs,
                              const location& rs_loc,
                              uint8_t rt,
                              const location& rt_loc,
                              int64_t addr,
                              const location& addr_loc,
                              QISA_Driver::BranchCondition cond)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: B" << _branchConditionNames[cond]
                   << "(rs=" << (int)rs << ",rt=" << (int)rt << ",addr=" << addr << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_CMP(inst_loc, rs, rs_loc, rt, rt_loc);

  if (result)
  {
    // Mark the fact that this branch is the result of an alias.
    result = generate_BR(inst_loc, cond, inst_loc, addr, addr_loc, true);
  }

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

/* ALIAS: [MOV rd,rs] --> [LDI rd,0 ; ADD rd,rs,rd] */

bool
QISA_Driver::generate_COPY(const location& inst_loc,
                          uint8_t rd,
                          const location& rd_loc,
                          uint8_t rs,
                          const location& rs_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: COPY(rd=" << (int)rd << ",rs=" << (int)rs << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.
  bool result = generate_XXX_rd_rs_rt("OR", inst_loc, rd, rd_loc, rs, rs_loc, rs, rs_loc);

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

bool QISA_Driver::generate_MOV(const location& inst_loc,
                                uint8_t rd,
                                const location& rd_loc,
                                int64_t imm,
                                const location& imm_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: MOV(rd=" << (int)rd << ",imm=" << imm << ");" << std::endl;


  // The 'imm' value is encoded using 32 bits (signed).
  // Check if the given value is within this range.

  const int64_t minImm = -(1LL<<31) + 1;
  const int64_t maxImm =  (1LL<<31) - 1;

  if (!checkValueRange(imm, minImm, maxImm, "imm", imm_loc))
  {
    return false;
  }

  bool result = false;

  const int64_t minImm20 = -(1LL<<19) + 1;
  const int64_t maxImm20 =  (1LL<<19) - 1;

  if (imm < minImm20 || imm > maxImm20)
  {
    // If the immediate doesn't fit in a 20 bit signed integer, we must split the given value into a lower and an upper part.
    // The lower part will have 17 bits and the upper part will have the rest (15 bits).
    const int64_t lowerPart = imm & U_IMM17_MASK;
    const int64_t upperPart = ((imm & ~U_IMM17_MASK) >> 17) & U_IMM15_MASK;

    result = generate_LDI(inst_loc, rd, rd_loc, lowerPart, imm_loc);

    if (result)
    {
      result = generate_LDUI(inst_loc, rd, rd_loc, upperPart, imm_loc);
    }
  }
  else
  {
    // We can use the immediate value as is and we'll be done.
    result = generate_LDI(inst_loc, rd, rd_loc, imm, imm_loc);
  }

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;

  return result;

}

/* ALIAS: [MULT2 rd,rs] --> [ADD rd,rs,rs] */
bool
QISA_Driver::generate_MULT2(const location& inst_loc,
                            uint8_t rd,
                            const location& rd_loc,
                            uint8_t rs,
                            const location& rs_loc)
{
  if (_verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- ALIAS: MULT2(rd=" << (int)rd << ",rs=" << (int)rs << ");" << std::endl;

  // We leave checking the parameters up to the generation functions we call.

  bool result = generate_XXX_rd_rs_rt("ADD", inst_loc, rd, rd_loc, rs, rs_loc, rs, rs_loc);

  if (result && _verbose)
      std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
                << "-- END ALIAS" << std::endl;
  return result;
}

bool
QISA_Driver::checkRegisterNumber(uint8_t reg_nr, const QISA::location& register_nr_loc, RegisterKind register_kind)
{
  if (reg_nr >= _nrOfRegisters[register_kind])
  {
    _errorStream << register_nr_loc << ": register nr (" << (int)reg_nr << ") too high, max="
                 << _nrOfRegisters[register_kind] - 1 << std::endl;
    _errorLoc = register_nr_loc;
    return false;
  }

  return true;
}


bool
QISA_Driver::checkValueRange(int64_t val,
                             int64_t minVal,
                             int64_t maxVal,
                             const std::string& val_name,
                             const QISA::location& val_loc)
{
    if ((val < minVal) || (val > maxVal))
    {
      _errorStream << val_loc << ": "
                   << val_name
                   << " (" << val << ") too large, min="
                   << minVal << ", max=" << maxVal << std::endl;
      _errorLoc = val_loc;
      return false;
    }

    return true;
}

bool
QISA_Driver::validate_qubit_address(uint8_t qubit_address,
                                        const location& loc)
{
  if (qubit_address > (_totalNrOfQubits - 1))
  {
    _errorStream << loc << ": Invalid qubit number used. Valid range: [0-"
                 << (_totalNrOfQubits - 1) << "]" << std::endl;
    _errorLoc = loc;
    return false;
  }
  return true;
}


bool
QISA_Driver::validate_s_mask(const std::vector<uint8_t>& s_mask,
                             const location& s_mask_loc)
{
  // A valid s_mask:
  //   - contains at least one element,
  //   - contains at most _totalNrOfQubits elements,
  //   - has no duplicates

  // It is assumed here that the qubit values given in s_mask have already been validated.
  if (s_mask.empty())
  {
    _errorStream << s_mask_loc << ": Need at least one bit in s_mask" << std::endl;
    _errorLoc = s_mask_loc;
    return false;
  }

  if ((int)s_mask.size() > _totalNrOfQubits)
  {
    _errorStream << s_mask_loc << ": to many bits in s_mask: max=" << _totalNrOfQubits << std::endl;
    _errorLoc = s_mask_loc;
    return false;
  }

  // Check for duplicates
  std::set<uint8_t> prev_values;
  for (auto it = s_mask.begin(); it != s_mask.end(); ++it)
  {
    if (prev_values.count(*it) == 0)
    {
      prev_values.insert(*it);
    }
    else
    {
      _errorStream << s_mask_loc << ": duplicate entry in s_mask: " << (int)*it << std::endl;
      _errorLoc = s_mask_loc;
      return false;
    }
  }

  return true;
}


bool
QISA_Driver::validate_t_mask(const std::vector<TargetControlPair>& t_mask,
                             const location& t_mask_loc)
{
  // A valid t_mask:
  //   - contains at least one element,
  //   - should not exceed the number of valid pairs,
  //   - has no duplicates
  //   - a qubit address can appear no more than once.

  // It is assumed here that the vector elements given in t_mask have already been validated.
  if (t_mask.empty())
  {
    _errorStream << t_mask_loc << ": Need at least one target control pair in t_mask" << std::endl;
    _errorLoc = t_mask_loc;
    return false;
  }

  if (t_mask.size() > _valid_target_control_pairs.size())
  {
    _errorStream << t_mask_loc << ": too many pairs in t_mask: max="
                 << _valid_target_control_pairs.size() << std::endl;
    _errorLoc = t_mask_loc;
    return false;
  }

  // Check for duplicates
  std::set<TargetControlPair> prev_values;
  for (auto it = t_mask.begin(); it != t_mask.end(); ++it)
  {
    if (prev_values.count(*it) == 0)
    {
      prev_values.insert(*it);
    }
    else
    {
      _errorStream << t_mask_loc << ": duplicate entry in t_mask: " << get_tc_pair_str(*it) << std::endl;
      _errorLoc = t_mask_loc;
      return false;
    }
  }

  // Ensure that each qubit only appears once in the list.
  std::set<uint8_t> prev_qubit_uses;
  for (auto it = t_mask.begin(); it != t_mask.end(); ++it)
  {
    size_t first_count = prev_qubit_uses.count(it->first);
    size_t second_count = prev_qubit_uses.count(it->second);
    if ((first_count  == 0) &&
        (second_count == 0))
    {
      prev_qubit_uses.insert(it->first);
      prev_qubit_uses.insert(it->second);
    }
    else
    {
      std::ostringstream ss;
      if ((first_count != 0) && (second_count != 0))
      {
        ss << ": qubits '" << (int)it->first << "' and '" << (int)it->second << "' are ";
      }
      else if ((first_count != 0))
      {
        ss << ": qubit '" << (int)it->first << "' is ";
      }
      else
      {
        ss << ": qubit '" << (int)it->second << "' is ";
      }

      _errorStream << t_mask_loc << ss.str()
                   << "used in more than one target-control pair in t_mask. Offending entry: "
                   << get_tc_pair_str(*it) << " (t_mask bit "
                   << (int)_valid_target_control_pairs[*it] << ") " << std::endl;
      _errorLoc = t_mask_loc;
      return false;
    }
  }

  return true;
}


bool QISA_Driver::validate_target_control_pair(const TargetControlPair& target_control_pair,
                                               const location& target_control_pair_loc)
{
  auto findIt = _valid_target_control_pairs.find(target_control_pair);

  if (findIt == _valid_target_control_pairs.end())
  {
    _errorStream << target_control_pair_loc << ": ("
                 << (int)target_control_pair.first << ","
                 << (int)target_control_pair.second
                 << ") is an invalid target-control pair" << std::endl;
    _errorLoc = target_control_pair_loc;
    return false;
  }

  return true;
}

bool
QISA_Driver::validate_bundle_separator(uint8_t bs_val,
                                       const location& bs_loc)
{
  if (!checkValueRange(bs_val, 0, 7, "BS", bs_loc))
  {
    return false;
  }

  return true;
}

QInstructionPtr
QISA_Driver::get_q_instr_arg_none(const std::string& inst_name,
                                  const location& inst_loc)
{
  int opcode;

  if (!get_opcode(inst_name, inst_loc, opcode, IK_DF_ARG_NONE))
  {
    return NULL;
  }

  return std::make_shared<QInstruction>(opcode);;
}

QInstructionPtr
QISA_Driver::get_q_instr_arg_st(const std::string& inst_name,
                                const location& inst_loc,
                                uint8_t st,
                                const location& st_loc,
                                bool is_conditional)
{
  int opcode;

  if (!get_opcode(inst_name, inst_loc, opcode, IK_DF_ARG_ST))
  {
    return NULL;
  }

  if (!checkRegisterNumber(st, st_loc, S_REGISTER))
  {
    return NULL;
  }

  return std::make_shared<QInstruction>(opcode, st, is_conditional);
}

QInstructionPtr
QISA_Driver::get_q_instr_arg_tt(const std::string& inst_name,
                                const location& inst_loc,
                                uint8_t tt,
                                const location& tt_loc)
{
  int opcode;

  if (!get_opcode(inst_name, inst_loc, opcode, IK_DF_ARG_TT))
  {
    return NULL;
  }

  if (!checkRegisterNumber(tt, tt_loc, T_REGISTER))
  {
    return NULL;
  }

  return std::make_shared<QInstruction>(opcode, tt);
}

QInstructionPtr
QISA_Driver::get_q_instr_arg_one(const std::string& inst_name,
                                 const location& inst_loc,
                                 const std::string& reg_name,
                                 const location& reg_name_loc)
{
  std::string searchString(inst_name);

  // Make the instruction name uppercase.
  // https://stackoverflow.com/a/17793588
  for (auto & c: searchString) c = toupper((unsigned char)c);


  auto stIt = _q_inst_arg_st_opcodes.find(searchString);

  if (stIt != _q_inst_arg_st_opcodes.end())
  {
    // This is a quantum instruction that expects an s-register.
    // The given reg_name should be an existing s-register alias.

    uint8_t reg_nr;
    bool success = get_register_nr(reg_name, reg_name_loc, QISA::QISA_Driver::S_REGISTER, reg_nr);
    if (success)
    {
      // By giving the false flag, we make sure that a s-register
      // type quantum instruction is created.
      return std::make_shared<QInstruction>(stIt->second, reg_nr, false);
    }
    else
    {
      // An error message has already been given by get_register_nr().
      // However, to keep consistent with the rest of the error messages,
      // add another one that specifically states that an s-register is expected.
      addExpectationErrorMessage("an S_REGISTER");
      return NULL;
    }
  }
  else
  {
    auto ttIt = _q_inst_arg_tt_opcodes.find(searchString);
    if (ttIt != _q_inst_arg_tt_opcodes.end())
    {
      // This is a quantum instruction that expects a t-register.
      // The given reg_name should be an existing t-register alias.

      uint8_t reg_nr;
      bool success = get_register_nr(reg_name, reg_name_loc, QISA::QISA_Driver::T_REGISTER, reg_nr);
      if (success)
      {
        return std::make_shared<QInstruction>(ttIt->second, reg_nr);
      }
      else
      {
        // An error message has already been given by get_register_nr().
        // However, to keep consistent with the rest of the error messages,
        // add another one that specifically states that an s-register is expected.
        addExpectationErrorMessage("a T_REGISTER");
        return NULL;
      }
    }
    else
    {
      _errorStream << inst_loc << ": opcode for quantum instruction '" << inst_name << "' not found" << std::endl;
      _errorLoc = inst_loc;
      return NULL;
    }
  }
}



uint64_t
QISA_Driver::encode_q_instr(const std::shared_ptr<QInstruction>& q_inst)
{
  uint64_t result;
  result = (q_inst->opcode & Q_INST_OPCODE_MASK) << Q_INST_OPCODE_OFFSET;

  switch (q_inst->type) {
    case QInstruction::ARG_NONE:
      break;
    case QInstruction::ARG_ST:
      result |= (q_inst->reg_nr & Q_INST_SD_MASK)
                | (q_inst->is_conditional & 1) << Q_INST_ST_COND_OFFSET;
      break;
    case QInstruction::ARG_TT:
      result |= (q_inst->reg_nr & Q_INST_TD_MASK);
      break;
    default:
      // Set to 0 in case another type has been given that we
      // don't now of yet...
      result = 0;
  }

  return result;
}


bool
QISA_Driver::decode_q_instr(uint64_t q_inst, std::string& q_inst_str)
{
  std::ostringstream ss;

  int opc = (q_inst >> Q_INST_OPCODE_OFFSET) & Q_INST_OPCODE_MASK;

  location errLoc = location();

  if (_quantumOpcode2instName.find(opc) == _quantumOpcode2instName.end())
  {
    _errorStream << "Unknown quantum opcode: " << getHex(opc, 2);
    _errorLoc = errLoc;
    ss << "<INVALID QUANTUM OPCODE: " << getHex(opc, 2) << ">";
    q_inst_str = ss.str();
    return false;
  }

  std::string inst_name = _quantumOpcode2instName[opc];

  if (_q_inst_arg_st_opcodes.find(inst_name) != _q_inst_arg_st_opcodes.end())
  {
    int rs = (q_inst & Q_INST_SD_MASK);
    if (!checkRegisterNumber(rs, errLoc, S_REGISTER))
      return false;

    bool is_cond = (q_inst >> Q_INST_ST_COND_OFFSET) & 1;

    if (is_cond)
    {
      ss << "C,";
    }

    ss << inst_name << " S" << rs;
  }
  else if (_q_inst_arg_tt_opcodes.find(inst_name) != _q_inst_arg_tt_opcodes.end())
  {
    int rt = (q_inst & Q_INST_TD_MASK);
    if (!checkRegisterNumber(rt, errLoc, T_REGISTER))
      return false;

    ss << inst_name << " T" << rt;
  }
  else
  {
    // If it is neither an st nor a tt instruction, it must be one without an argument.
    ss << inst_name;
  }

  q_inst_str = ss.str();

  return true;
}


bool
QISA_Driver::generate_q_bundle(uint8_t bs_val,
                               const location& bs_loc,
                               const BundledQInstructions& bundle,
                               const location& bundle_loc)
{
  if (_verbose)
  {
    std::cout << std::setw(8) << std::setfill('0') << _instructions.size() << ": " << std::setw(0)
              << "Q_BUNDLE: bs=" << (int)bs_val
              << ", bundle: (";

    auto it = bundle.begin();
    if (it != bundle.end())
    {
      std::cout << _quantumOpcode2instName[(*it)->opcode];
    }
    ++it;
    for (; it != bundle.end(); ++it)
    {
      std::cout << "," << _quantumOpcode2instName[(*it)->opcode];
    }
    std::cout << ")" << std::endl;
  }

  bool issued_bs = false;

  for (auto it = bundle.begin(); it != bundle.end(); ++it)
  {
    // Quantum instructions are put pair-wise in a
    // 'very large instruction word' (VLIW).

    // The double instruction format always start with the highest bit set.
    qisa_instruction_type instruction = (1L << DBL_INST_FORMAT_BIT_OFFSET);

    // Set the bundle separator. This comes at the last 3 bits.
    // Only do this for the first VLIW of the bundle.
    if (!issued_bs)
    {
      instruction |= (bs_val & BS_MASK);
      issued_bs = true;
    }

    // Handle the first pair of the VLIW.

    instruction |= (encode_q_instr(*it) << VLIW_INST_0_OFFSET);

    // Handle the second pair of the VLIW (if there are any quantum instructions left to encode).
    ++it;
    if (it != bundle.end())
    {
      instruction |= (encode_q_instr(*it) << VLIW_INST_1_OFFSET);
    }
    // This VLIW is done. Save it.
    _instructions.emplace_back(instruction);

    // Apparently, the compiler doesn't like when we mess around with the loop iterator ourselves...
    // It will not detect the end-of-vector properly.
    if (it == bundle.end())
      break;
  }

  return true;
}


std::string
QISA_Driver::get_s_mask_str(const std::vector<uint8_t>& s_mask)
{
  std::ostringstream ss;
  ss << "{";
  if (!s_mask.empty())
  {
    auto it = s_mask.begin();
    ss << (int)*it;
    it++;
    for (;it != s_mask.end(); ++it)
    {
      ss << ", " << (int)*it;
    }
  }
  ss << "}";

  return ss.str();
}

std::string
QISA_Driver::get_tc_pair_str(const TargetControlPair& tc_pair)
{
  std::ostringstream ss;
  ss << "(" << (int)tc_pair.first << "," << (int)tc_pair.second << ")";
  return ss.str();
}


std::string
QISA_Driver::get_t_mask_str(const std::vector<QISA_Driver::TargetControlPair>& t_mask)
{
  std::ostringstream ss;
  ss << "{";
  if (!t_mask.empty())
  {
    auto it = t_mask.begin();

    ss << get_tc_pair_str(*it);
    it++;
    for (;it != t_mask.end(); ++it)
    {
      ss << ", " << get_tc_pair_str(*it);
    }
  }
  ss << "}";

  return ss.str();
}


bool
QISA_Driver::processDeferredInstructions()
{
  // Assume that there are no errors...
  int result = true;

  if (!_deferredInstructions.empty())
  {
    if (_verbose)
      std::cout << "Processing deferred instructions..." << std::endl;
  }

  for (auto& itKV : _deferredInstructions)
  {
    if (itKV.second.label_name == "<<MULTI_LABEL_ERROR>>")
    {
      // An error has already been detected: an undeclared label has been used multiple times for the same instruction.
      // This is as of yet not supported.
      _errorStream << itKV.second.label_name_loc
                   << ": Using multiple labels for one instruction is not supported." << std::endl;
      _errorLoc = itKV.second.label_name_loc;

      // Set return value to false to indicate an error condition.
      result = false;

      // Continue with the next deferred instruction.
      continue;
    }

    auto findIt = _labels.find(itKV.second.label_name);
    if (findIt == _labels.end())
    {
      // Label has not been defined in this program.
      // Issue an error.

      _errorStream << itKV.second.label_name_loc
                   << ": Label '" << itKV.second.label_name << "' not found." << std::endl;
      _errorLoc = itKV.second.label_name_loc;

      // Set return value to false to indicate an error condition.
      result = false;

// We used to return multiple errors, but this breaks things with error reporting.
#if 0
      // Continue with the next deferred instruction.
      continue;
#endif
      // Now, we return at the first missing label.
      return result;
    }

    const uint64_t label_address = findIt->second;

    switch (itKV.second.instruction)
    {
      case QISA_Parser::token::TOK_BR:
      {
        int64_t offset = label_address - itKV.second.programCounter;

        // Ensure that the new offset is valid.

        // An offset is encoded using 21 bits (signed).
        // Check if the given value is within this range.

        const int64_t minOffset = -(1LL<<20) + 1;
        const int64_t maxOffset =  (1LL<<20) - 1;


        if (!checkValueRange(offset, minOffset, maxOffset, "addr", itKV.second.label_name_loc))
        {
          return false;
        }

        if (itKV.second.is_alias)
        {
          // Correct for the implicit CMP instruction that precedes the actual branch instruction.
          // Normally, the branch offset will always be positive, but just to be sure we handle
          // the negative case also.
          if (offset < 0)
          {
            offset--;
          }
          else
          {
            // The positive case is already been handled implicitely by the fact that
            // the program counter already advanced by one due to the CMP instruction.
          }
        }


        // We only have to set the offset, so we 'or' the affected instruction with this offset.
        _instructions[itKV.second.programCounter] |= ((offset & ADDR_MASK) << ADDR_OFFSET);

        if (_verbose)
        {
          std::cout << "Resolved label offset for instruction " << itKV.second.programCounter
                    << " to " << offset << std::endl;
        }

        break;
      }
      default:
      {
        _errorStream << itKV.second.label_name_loc
                     << ": Use of forward defined label '" << itKV.second.label_name
                     << "' for this instruction is not yet supported!" << std::endl;
        _errorLoc = itKV.second.label_name_loc;
        // Set return value to false to indicate an error condition.
        result = false;

        break;
      }
    }
  }

  return result;
}


std::vector<std::string>
QISA_Driver::getInstructionsAsHexStrings(bool withBinaryOutput)
{
  std::vector<std::string> result;

  if (withBinaryOutput)
  {
    for (auto& it : _instructions)
    {
      result.push_back(getHex(it, 8) + " (" + getSpacedBinary(it) + ")");
    }
  }
  else
  {
    for (auto& it : _instructions)
    {
      result.push_back(getHex(it, 8));
    }
  }

  return result;
}

bool
QISA_Driver::setDisassemblyFormat(int format_id)
{
  if (format_id < 1 || format_id > 2)
  {
    error("Incorrect format_id. Allowed is either 1 or 2.");
    // Return false to indicate failure;
    return false;
  }

  _disassemblyFormatId = format_id;
  return true;
}

std::string QISA_Driver::getDisassemblyOutput()
{
  if (_lastDriverAction != DRIVER_ACTION_DISASSEMBLE)
  {
    return "Can only get disassembly output after successful disassembly!";
  }

  // Determine the longest assembly output line.
  // This will be used to properly indent the instruction
  // hex code comment in output format 2.

  size_t maxDisassemblyLineLength = 0;
  if (_disassemblyFormatId == 2)
  {
    for (auto it : _disassembledInstructions)
    {
      if (maxDisassemblyLineLength < it.second.instruction.size())
      {
        maxDisassemblyLineLength = it.second.instruction.size();
      }
    }
    // Add 4 to leave some space between the end of the instruction text
    // and the start of the hex code.
    maxDisassemblyLineLength += 4;
    // Add the label string length to it.
    // This will be zero when no branch instructions were used.
    maxDisassemblyLineLength += _disassemblyLabelStringLength;
  }


  // This will hold the disassembly output to return.
  std::stringstream disassemblyOutput;

  if (_disassemblyLabels.empty())
  {
    // This is the simple (but unlikely) case where there are no branch
    // instructions in the code.

    if (_verbose)
      std::cout << "No branch instructions found." << std::endl;

    if (_disassemblyFormatId == 1)
    {
      for (auto it : _disassembledInstructions)
      {
        disassemblyOutput << it.second.hexCode << "  # " << it.second.instruction << std::endl;
      }
    }
    else // For now there are only two output formats, so this must be format 2.
    {
      for (auto it : _disassembledInstructions)
      {
        disassemblyOutput << std::setw(maxDisassemblyLineLength) << std::left
                  << it.second.instruction << "# "
                  << it.second.hexCode << std::endl;
      }
    }
  }
  else
  {
    if (_disassemblyFormatId == 1)
    {
      for (auto it : _disassembledInstructions)
      {
        disassemblyOutput << it.second.hexCode << "  # " << it.second.label
                  << it.second.instruction << std::endl;
      }
    }
    else // For now there are only two output formats, so this must be format 2.
    {
      for (auto it : _disassembledInstructions)
      {
        std::string fullInstruction(it.second.label + it.second.instruction);
        disassemblyOutput << std::setw(maxDisassemblyLineLength) << std::left
                  << fullInstruction << "# " << it.second.hexCode << std::endl;
      }
    }
  }

  return disassemblyOutput.str();
}

bool
QISA_Driver::saveAssembly(std::ofstream& outputStream)
{
  if (_instructions.empty())
  {
    error("Nothing to save. Have you called parse()?");
    // Return false to indicate failure;
    return false;
  }

  for (auto& it : _instructions)
  {
    const qisa_instruction_type instruction = it;
    outputStream.write(reinterpret_cast<const char*>(&instruction), sizeof(qisa_instruction_type));

    if (outputStream.fail())
    {
      error("Error occurred while writing assembly output to output stream.");
      return false;
    }
  }

  // Return true to indicate success;
  return true;
}

bool
QISA_Driver::saveAssembly(const std::string& outputFileName)
{
  if (_instructions.empty())
  {
    error("Nothing to save. Have you called assemble()?");
    // Return false to indicate failure;
    return false;
  }

  std::ofstream outputFileStream(outputFileName, std::ios::out | std::ios::binary);
  if (outputFileStream.fail())
  {
    _errorStream << "Cannot open file '" << outputFileName << "' for writing" << std::endl;
    _errorLoc = location();
    // Return false to indicate failure;
    return false;
  }

  if (!saveAssembly(outputFileStream))
  {
    _errorStream << "Write error on file '" << outputFileName << "'" << std::endl;
    _errorLoc = location();
    // Return false to indicate failure;
    return false;
  }

  // Return true to indicate success;
  return true;
}

bool
QISA_Driver::saveDisassembly(std::ofstream& outputStream)
{
  std::string disassemblyOutput = getDisassemblyOutput();

  outputStream << disassemblyOutput;
  if (outputStream.fail())
  {
    error("Error occurred while writing disassembly output to output stream");

    // Return false to indicate failure;
    return false;
  }

  // Return true to indicate success;
  return true;
}

bool
QISA_Driver::saveDisassembly(const std::string& outputFileName)
{
  std::ofstream outputFileStream(outputFileName, std::ios::out);
  if (outputFileStream.fail())
  {
    _errorStream << "Cannot open file '" << outputFileName << "' for writing" << std::endl;
    _errorLoc = location();
    // Return false to indicate failure;
    return false;
  }

  if (!saveDisassembly(outputFileStream))
  {
    _errorStream << "Write error on file '" << outputFileName << "'" << std::endl;
    _errorLoc = location();
    // Return false to indicate failure;
    return false;
  }

  // Return true to indicate success;
  return true;
}

bool
QISA_Driver::save(std::ofstream& outputStream)
{
  bool result = false;

  switch(_lastDriverAction)
  {
    case DRIVER_ACTION_PARSE:
      result = saveAssembly(outputStream);
      break;

    case DRIVER_ACTION_DISASSEMBLE:
      result = saveDisassembly(outputStream);
      break;

    case DRIVER_ACTION_NONE:
      _errorStream << "Nothing to save. Have you called parse() or disassemble(), "
                      "or perhaps there was an error?" << std::endl;
      _errorLoc = location();

      // Return false to indicate failure;
      result = false;
      break;
  }

  return result;
}

bool
QISA_Driver::save(const std::string& outputFileName)
{
  bool result = false;

  switch(_lastDriverAction)
  {
    case DRIVER_ACTION_PARSE:
      result = saveAssembly(outputFileName);
      break;

    case DRIVER_ACTION_DISASSEMBLE:
      result = saveDisassembly(outputFileName);
      break;

    case DRIVER_ACTION_NONE:
      _errorStream << "Nothing to save. Have you called parse() or disassemble(), "
                      "or perhaps there was an error?" << std::endl;
      _errorLoc = location();

      // Return false to indicate failure;
      result = false;
      break;
  }

  return result;
}

bool
QISA_Driver::loadQuantumInstructions(const std::string& qMapFilename)
{
  // Create an object that can load the map file.
  QISA_QMapParser qMapParser;

  bool success = qMapParser.parse(qMapFilename);

  if (!success)
  {
    _errorStream << "Error parsing QMAP file '" << qMapFilename <<  "':" << std::endl;
    _errorStream << "\t" << qMapParser.getLastErrorMessage() << std::endl;
    _errorLoc = location();

    return false;
  }

  q_map_t arg_none_map;
  q_map_t arg_st_map;
  q_map_t arg_tt_map;

  // Note: we know the previous step was successful.
  // That's why we can do without checking the result of getInstructionCodeMaps.
  qMapParser.getInstructionCodeMaps(arg_none_map, arg_st_map, arg_tt_map);


  return loadQuantumInstructions(arg_none_map,
                                arg_st_map,
                                arg_tt_map);
}


bool
QISA_Driver::checkQuantumInstructionMap(const std::string& mapName,
                                        const q_map_t& inputMap,
                                        std::map<std::string, std::string>& checkInstructionMap,
                                        std::map<int, std::string>& checkOpcodeMap,
                                        q_map_t& outputMap)
{
  auto itCheckOpc = checkOpcodeMap.begin();
  auto itCheckInstr = checkInstructionMap.begin();

  for(auto it : inputMap)
  {
    // First, check that the given opcode is within range.
    if (it.second < 0)
    {
      _errorStream << "Negative opcode not allowed: "
                   << mapName << "['" << it.first << "'] = "
                   << it.second;
      _errorLoc = location();
      return false;
    }
    else if (it.second > _maxQuantumOpcodeVal)
    {
      _errorStream << "Opcode value too high (max=" << _maxQuantumOpcodeVal
                   << mapName << "['" << it.first << "'] = (" << it.second << "/"
                   << getHex(it.second, 2)
                   << ")";
      _errorLoc = location();
      return false;
    }

    // Now check if the opcode has been used in another map.
    itCheckOpc = checkOpcodeMap.find(it.second);
    if (itCheckOpc != checkOpcodeMap.end())
    {
      _errorStream << "Opcode (" << it.second << "/"
                   << getHex(it.second, 2)
                   << ") specified for "
                   << mapName << "['" << it.first
                   << "'] is already used in " <<
                   itCheckOpc->second << std::endl;
      _errorLoc = location();
      return false;
    }

    // The opcode seems OK.
    // Put the combination into checkOpcodeMap so that it can be
    // checked against for other instructions.
    checkOpcodeMap[it.second] = mapName + "['" + it.first + "']";

    // Make the instruction name uppercase.
    // https://stackoverflow.com/a/17793588
    std::string instructionName = it.first;
    for (auto & c: instructionName) c = toupper((unsigned char)c);


    // Check if the instruction name is already used in another map.
    itCheckInstr = checkInstructionMap.find(instructionName);
    if (itCheckInstr != checkInstructionMap.end())
    {
      _errorStream << "Instruction "
                   << mapName << "['" << instructionName
                   << "'] is already used in"
                   << itCheckInstr->second << std::endl;
      _errorLoc = location();
      return false;
    }

    // The instruction name seems OK.
    // Put the combination into checkInstructionMap so that it can be
    // checked against for other instructions.
    checkInstructionMap[instructionName] = mapName + "['" + instructionName + "']";


    // Put the validated result in outputMap.
    outputMap[instructionName] = it.second;
  }

  return true;
}

bool
QISA_Driver::loadQuantumInstructions(const q_map_t& arg_none_map,
                                    const q_map_t& arg_st_map,
                                    const q_map_t& arg_tt_map)
{
  // Will hold the validated results.
  q_map_t q_inst_arg_none_opcodes;
  q_map_t q_inst_arg_st_opcodes;
  q_map_t q_inst_arg_tt_opcodes;


  // Do not assume that the given maps are OK.

  // This map will be used to ensure that there are no
  // duplicate opcodes between the given maps.
  std::map<int, std::string> checkOpcodeMap;

  // This map will be used to ensure that there are no
  // duplicate instruction names between the given maps.
  std::map<std::string, std::string> checkInstructionMap;

  bool result = checkQuantumInstructionMap("arg_none_map",
                                           arg_none_map,
                                           checkInstructionMap,
                                           checkOpcodeMap,
                                           q_inst_arg_none_opcodes);


  // Check if  opcode '0' is present in 'arg_none_map'.

  // This instruction is used as filler when only one double format
  // instruction has been specified on an assembly instruction.
  bool foundOpcodeZero = false;
  for (auto it : q_inst_arg_none_opcodes)
  {
    if (it.second == 0)
    {
      foundOpcodeZero = true;
      break;
    }
  }

  if (!foundOpcodeZero)
  {
    _errorStream << "The Quantum instruction for opcode 0 is missing." << std::endl;
    _errorStream << "It is mandatory for correct operation of QISA-AS."  << std::endl;
    _errorStream << "Specify this in parameter 'arg_none_map'."  << std::endl;
    return false;
  }

  if (result == false)
  {
    // An error message already has been left.
    return false;
  }

  result = checkQuantumInstructionMap("arg_st_map",
                                      arg_st_map,
                                      checkInstructionMap,
                                      checkOpcodeMap,
                                      q_inst_arg_st_opcodes);
  if (result == false)
  {
    // An error message already has been left.
    return false;
  }

  result = checkQuantumInstructionMap("arg_tt_map",
                                      arg_tt_map,
                                      checkInstructionMap,
                                      checkOpcodeMap,
                                      q_inst_arg_tt_opcodes);
  if (result == false)
  {
    // An error message already has been left.
    return false;
  }

  // Everything seems fine.
  // Now update the appropriate member maps.
  _q_inst_arg_none_opcodes.swap(q_inst_arg_none_opcodes);
  _q_inst_arg_st_opcodes.swap(q_inst_arg_st_opcodes);
  _q_inst_arg_tt_opcodes.swap(q_inst_arg_tt_opcodes);


  // For disassembly purposes, we also need the reverse map,
  // which maps an opcode to an instruction name.
  // Process all maps in sequence.
  _quantumOpcode2instName.clear();

  for (auto it : _q_inst_arg_none_opcodes)
  {
    _quantumOpcode2instName[it.second] = it.first;
  }

  for (auto it : _q_inst_arg_st_opcodes)
  {
    _quantumOpcode2instName[it.second] = it.first;
  }

  for (auto it : _q_inst_arg_tt_opcodes)
  {
    _quantumOpcode2instName[it.second] = it.first;
  }

  return true;
}

} // namespace QISA

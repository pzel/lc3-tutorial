#!/usr/bin/env ruby
require 'minitest/autorun'
require 'tempfile'

class Run
  def initialize vm, source_lines
    @assembly_output, obj_path = assemble(source_lines.join("\n"))
    @res_string = vm.run_binary(obj_path)
    `rm -f #{obj_path}`
  end

  def registers
    clean_string = @res_string.gsub(/.*--- halting the LC-3 ---/m, '')
    parse_registers(clean_string)
  end

  @private
  def parse_registers(register_list)
    ## We expect this format for the register listing
    /R0=x(.{4}) R1=x(.{4}) R2=x(.{4}) R3=x(.{4}) R4=x(.{4}) R5=x(.{4}) R6=x(.{4}) R7=x(.{4})/
      .match(register_list)
      .to_a
      .slice(1, 8)
  end

end

def assemble code
  # The assembler from
  # https://github.com/haplesshero13/lc3tools
  f = Tempfile.new(['lc3_src', '.asm'])
  f.write(code+"\n")
  f.close
  object_path = f.path.gsub('.asm', '.obj')
  assembly_output = `lc3as #{f.path}`
  f.unlink
  [assembly_output, object_path]
end

class OriginalVM
  # The canonical LC-3 simulator
  # https://github.com/haplesshero13/lc3tools
  def self.run_binary object_path
    script = Tempfile.new(['lc3_script', '.sh'])
    script.write(["file #{object_path}", "continue", "quit"].join("\n"))
    script.close
    run_result = `lc3sim -s #{script.path}`
    script.unlink
    run_result
  end
end


class MyVM
  def self.run_binary object_path
    `./lc #{object_path}`
  end
end

class RegisterStateTests < Minitest::Test

  def test_anding_register_with_zero_zeroes_it_out
    code = [".ORIG x3000",
            "AND R5, R5, 0",
            "HALT",
            ".END"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[5], "0000")
    assert_equal(want[5], got[5])
  end

  def test_adding_one_to_zeored_register
    code = [".ORIG x3000",
            "AND R5, R5, 0",
            "ADD R5, R5, 1",
            "HALT",
            ".END"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[5], "0001")
    assert_equal(want[5], got[5])
  end

  def test_adding_one_twice_gives_2
    code = [".ORIG x3000",
            "AND R5, R5, 0",
            "ADD R5, R5, 1",
            "ADD R5, R5, 1",
            "HALT",
            ".END"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[5], "0002")
    assert_equal(want[5], got[5])
  end

  def test_ld
    code = [".ORIG x3000",
            "LD R5, LETTER_A",
            "HALT",
            "LETTER_A: .FILL 65",
            ".END"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[5], "0041")
    assert_equal(want[5], got[5])
  end

  def test_an_implementation_of_xor
    code = [".ORIG x3000",
            "AND R1, R1, 0",
            "AND R2, R2, 0",
            "ADD R0, R0, 1",
            "ADD R1, R0, 2",
            ## R3 <- XOR(R1,R2)
            "NOT R1,R1",
            "AND R3,R1,R2",
            "NOT R1,R1",
            "NOT R2,R2",
            "AND R4,R1,R2",
            "NOT R2,R2",
            "NOT R3,R3",
            "NOT R4,R4",
            "AND R3,R3,R4",
            "NOT R3,R3",
            "HALT"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[3], "0003")
    assert_equal(got[3], "0003")
  end


end

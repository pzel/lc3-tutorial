#!/usr/bin/env ruby
require 'minitest/autorun'
require 'tempfile'

class Run
  def initialize vm, source_lines
    @assembly_output, obj_path = assemble(source_lines.join("\n"))
    @res_string = vm.run_binary(obj_path)
    `rm -f #{obj_path}`
#    puts "res string", @res_string
  end

  def registers
    clean_string = @res_string.gsub(/.*--- halting the LC-3 ---/m, '')
    parse_registers(clean_string)
  end

  @private
  def parse_registers(register_list)
    /R0=x(.{4}) R1=x(.{4}) R2=x(.{4}) R3=x(.{4}) R4=x(.{4}) R5=x(.{4}) R6=x(.{4}) R7=x(.{4})/
      .match(register_list)
      .to_a
      .slice(1, 8)
#      map{|x| x.to_i(16)}
  end

end

def assemble code
  f = Tempfile.new(['lc3_src', '.asm'])
  f.write(code+"\n")
  f.close
  object_path = f.path.gsub('.asm', '.obj')
  assembly_output = `lc3as #{f.path}`
  f.unlink
  [assembly_output, object_path]
end

class OriginalVM
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
    code = [".ORIG x3000", "AND R5, R5, 0", "HALT", ".END"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[5], "0000")
    assert_equal(want[5], got[5])
  end

  def test_adding_one_to_zeored_register
    code = [".ORIG x3000", "AND R5, R5, 0", "ADD R5, R5, 1", "HALT", ".END"]
    want = Run.new(OriginalVM, code).registers
    got = Run.new(MyVM, code).registers
    assert_equal(want[5], "0001")
    assert_equal(want[5], got[5])
  end
end

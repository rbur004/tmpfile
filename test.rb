#!/usr/local/bin/ruby

require 'tmpfile'

# t = TmpFile.new("/tmp/pp","w")
# puts t.to_s
# t.puts "Hello World"

TmpFileMod::TmpFile.open('/tmp/xxx', 'w+') do |fd|
  fd.no_unlink # Remove and the test file will be deleted on close.
  fd.puts "Hello World #{Time.now}", 10
  fd.print 'Another'
  fd.print "Line\n"
  fd.rewind
  fd.each_line do |l|
    puts l
  end
end

#!/usr/local/bin/ruby

require 'tmpfile'
include TmpFileMod


=begin
t = TmpFile.new("/tmp/pp","w")
puts t.to_s
t.puts "Hello World"
=end

TmpFile.open('/tmp/xxx','w+') do |fd|
  #fd.no_unlink #Uncomment and the test file will not be deleted.
  fd.puts "Hello World #{Time.now}", 10
  fd.print "Another"
  fd.print "Line\n"
  fd.rewind
  fd.each_line do |l|
    puts l
  end
end

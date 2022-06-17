#Tmpfile

Creates temporary files, that get deleted after they are closed (unless no_unlink is set).

##Install
To make the make file
ruby extconf.rb

The makefile will have the ruby library path added for the
version of ruby used to create the makefile.

To create the ruby library
% make

To install in the current ruby instance
% make install

```
ruby extconf.rb
make
sudo make install
```

##Example
```
<<<<<<< HEAD
	#Create a named temporary file, that gets deleted at the end of the block
	TmpFileMod::TmpFile.open('/tmp/xxx','w+') do |fd|
		...
	end

	#For debugging, the delete on close can be disabled.
	TmpFile.open('/tmp/xxx','w+') do |fd|
    	#fd.no_unlink; #For testing, so we can see what was produced.
		...
	end
=======
require 'tmpfile'
include

# Create a named temporary file, that gets deleted at the end of the block
TmpFileMod::TmpFile.open('/tmp/xxx','w+') do |fd|
	...
end

# For debugging, the delete on close can be disabled.
TmpFileMod::TmpFile.open('/tmp/xxx','w+') do |fd|
  #fd.no_unlink; #For testing, so we can see what was produced.
	...
end
>>>>>>> e7d5a5faa8b4f9c353ecc1f35559f6fc12b40761
```

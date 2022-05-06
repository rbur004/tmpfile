#include <sys/types.h>
#include <sys/wait.h>

#include <ruby.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "tmpfile.h"


static VALUE tmp_unlink(VALUE obj);

static VALUE myModule;
static VALUE myClass;

static void tmp_mark(Tmpfile *tf)
{
  //don't need to do anything.
}

static void tmp_free(Tmpfile *tf)
{
  if(tf)
  {
    if(tf->fp)
      fclose(tf->fp);
    free(tf);
  }
}

static VALUE tmp_alloc(VALUE klass)
{
Tmpfile *tf = calloc(1, sizeof(Tmpfile));

  return Data_Wrap_Struct(klass, tmp_mark, tmp_free, tf); //wraps a c struct in VALUE
  //recover with Data_Get_Struct()
}

static VALUE tmp_initialize(int argc, VALUE *argv, VALUE obj)
{
  VALUE filename, flags;
Tmpfile *tf;
FILE *fp;
char *filename_p, *flags_p;

  rb_scan_args(argc, argv, "2", &filename, &flags);
  if(TYPE(filename) != T_STRING)
    rb_raise(rb_eTypeError, "tmp_initialize: Filename should be a String");

  if(TYPE(flags) != T_STRING)
    rb_raise(rb_eTypeError, "tmp_initialize: Flags should be a String");

  Data_Get_Struct(obj, Tmpfile, tf);

  filename_p = StringValuePtr(filename);
  flags_p = StringValuePtr(flags);
  if( ( fp = fopen(filename_p, flags_p)) == NULL)
    rb_raise(rb_eArgError, "tmp_initialize: %s", strerror(errno));

  tf->fp = fp;
  strcpy(tf->filename, filename_p);
  strcpy(tf->flags, flags_p);

  return obj;
}

static VALUE tmp_close(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);
  if(tf->fp != NULL)
  {
    fclose(tf->fp); //close the file
    tf->fp = NULL;
    if(tf->no_unlink == 0)
      tmp_unlink(obj); //remove the file from disk
  }
  return obj;
}

//Class level method, alternate to new.
static VALUE tmp_open(int argc, VALUE *argv, VALUE obj)
{
VALUE nobj = rb_class_new_instance( argc, argv, obj); //new instance of this class.

  if(rb_block_given_p()) //if we have a block.
  { rb_yield(nobj);
    tmp_close(nobj);
  }
  return nobj;
}

static VALUE tmp_each_line(VALUE obj, VALUE filename, VALUE flags)
{
  Tmpfile *tf;
  char buffer[8196];

  Data_Get_Struct(obj, Tmpfile, tf);

  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_each_line: File not open!"); //need to change error type.

  if(rb_block_given_p()) //if we have a block.
  {
    while(fgets(buffer,8192,tf->fp) != NULL) //read the next line.
      rb_yield(rb_str_new2(buffer)); //pass that line to the block.
    //we should have EOF.
    if(feof(tf->fp) == 0) //Then we had an io error, rather than eof.
      rb_raise(rb_eRuntimeError, "tmp_each_line: %s", strerror(errno)); //need to change error type.
  }
  else
    rb_raise(rb_eRuntimeError, "tmp_each_line: no block given"); //need to change error type.

  return obj; //return ourselves.
}

//Read a line. Max line size is 8192.
static VALUE tmp_gets(VALUE obj)
{
  Tmpfile *tf;
  char buffer[8194];

  Data_Get_Struct(obj, Tmpfile, tf);

  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_gets: File not open!"); //need to change error type.

  if(fgets(buffer,8192,tf->fp) == NULL) //read the next line.
  {
    if(feof(tf->fp) == 0) //Then we had an io error, rather than eof.
      rb_raise(rb_eRuntimeError, "tmp_gets: %s", strerror(errno)); //need to change error type.
    else
      return Qnil; //nothing more to read.
  }
  return rb_str_new2(buffer); //return the string we just read.
}

//Mimic Ruby puts, adding a '\n' if the strings don't have one.
static VALUE tmp_puts(int argc, VALUE *argv, VALUE obj)
{
  Tmpfile *tf;
  int i;
  VALUE tmp;

  Data_Get_Struct(obj, Tmpfile, tf);

  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_puts: File not open!"); //need to change error type.

  for(i = 0; i < argc; i++)
  {
    char *s;
    int l;

    if(TYPE(argv[i]) == T_STRING)
      s = StringValuePtr(argv[i]);
    else
    { tmp = rb_funcall( argv[i],  rb_intern("to_s"),  0 );
      s = StringValuePtr(tmp);
    }

    l = (int)strlen(s);
    //only output if the string isn't empty.
    if(l && fputs(s, tf->fp) == EOF)
      rb_raise(rb_eRuntimeError, "tmp_puts: %s", strerror(errno)); //need to change error type.
    //Add a new line char if the string didn't end in one.
    if(l && s[l-1] != '\n' && fputs("\n", tf->fp) == EOF)
      rb_raise(rb_eRuntimeError, "tmp_puts: %s", strerror(errno)); //need to change error type.
  }

  return obj;
}

static VALUE tmp_print(int argc, VALUE *argv, VALUE obj)
{
  Tmpfile *tf;
  int i;
  VALUE tmp;

  Data_Get_Struct(obj, Tmpfile, tf);

  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_print: File not open!"); //need to change error type.

  for(i = 0; i < argc; i++)
  {
    char *s;

    if(TYPE(argv[i]) == T_STRING)
      s = StringValuePtr(argv[i]);
    else
    { tmp = rb_funcall( argv[i],  rb_intern("to_s"),  0 );
      s = StringValuePtr(tmp);
    }

    //only output if the string isn't empty.
    if(s[0] != '\0' && fputs(s, tf->fp) == EOF)
      rb_raise(rb_eRuntimeError, "tmp_print: %s", strerror(errno)); //need to change error type.
  }

  return obj;
}

static VALUE tmp_seek_end(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);
  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_seek_end: File not open!"); //need to change error type.

  if(fseek(tf->fp,0,SEEK_END) == -1)  //go to the end of the file.
    rb_raise(rb_eRuntimeError, "tmp_seek_end.fseek %s", strerror(errno));
  return obj;
}

static VALUE tmp_flush(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);
  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_flush: File not open!"); //need to change error type.

  if(fflush(tf->fp)  == -1)
    rb_raise(rb_eRuntimeError, "tmp_flush.flush: %s", strerror(errno));
  return obj;
}

//rewind to start of file, or if arg, back arg bytes.
static VALUE tmp_rewind(int argc, VALUE *argv, VALUE obj)
{
  Tmpfile *tf;
  VALUE nbytes;

  Data_Get_Struct(obj, Tmpfile, tf);
  if(tf->fp == NULL)
    rb_raise(rb_eRuntimeError, "tmp_rewind: File not open!"); //need to change error type.

  if(tf->fp != NULL){
    if(argc == 0) {
      if(fseek(tf->fp, 0, SEEK_SET) == -1)  //go back to the start of the file.
        rb_raise(rb_eRuntimeError, "tmp_rewind.rewind %s", strerror(errno));
    }
    else {
      //rewind n bytes in the input stream.
      rb_scan_args(argc, argv, "10", &nbytes);
      if(fseek(tf->fp, NUM2INT(nbytes), SEEK_CUR) == -1)
        rb_raise(rb_eRuntimeError, "tmp_rewind.rewind %s", strerror(errno));
    }
  }
  return obj;
}

static VALUE tmp_set_no_unlink(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);
  tf->no_unlink = 1;
  return obj;
}

static VALUE tmp_set_unlink(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);
  tf->no_unlink = 0;
  return obj;
}

static VALUE tmp_do_unlink(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);
  return tf->no_unlink == 0 ? Qtrue:Qfalse ;
}

static VALUE tmp_unlink(VALUE obj)
{
  Tmpfile *tf;

  Data_Get_Struct(obj, Tmpfile, tf);

  if(tf->filename[0] != '\0')
  {
    unlink(tf->filename); //remove the file from disk

    //Forget the name, as the file is now unnamed.
    //i.e. it can only be accessed via the file pointer, not by name.
    //     and it will vanish from the disk once the fp is closed.
    tf->filename[0] = '\0';
    tf->flags[0] = '\0';
  }
  return obj;
}

static VALUE tmp_to_s(VALUE obj)
{
  char *buffer;
  int fd;
  Tmpfile *tf;
  VALUE rb_string;

  Data_Get_Struct(obj, Tmpfile, tf);
  if(tf->fp == NULL)
    fd = -1;
  else
    fd = fileno(tf->fp);

  if((buffer = malloc(strlen(tf->filename) + strlen(tf->flags) + 30)) == NULL)
    rb_raise(rb_eRuntimeError, "Malloc failed");

  sprintf(buffer, "filename = \'%s\' flags = \'%s\'", tf->filename, tf->flags);
  rb_string = rb_str_new2(buffer); // return the string we just read.
  free(buffer); // rb_string is a copy, so we don't need the original.
  return rb_string;
}


static VALUE tmp_exec(int argc, VALUE *argv, VALUE obj)
{
char **exec_argv;
int i;
int status = 0;
int pid;
int error;
VALUE tmp;

  if(argc == 0)
        rb_raise(rb_eRuntimeError, "tmp_exec: %s", "Need arguments!");

  exec_argv = calloc(argc + 1, sizeof(char *));
  for(i = 0; i < argc; i++)
  { if(TYPE(argv[i]) == T_STRING)
      exec_argv[i] = StringValuePtr(argv[i]);
    else
    { tmp = rb_funcall( argv[i],  rb_intern("to_s"),  0 ) ;
      exec_argv[i] = StringValuePtr(tmp);
    }
  }
  exec_argv[i] = NULL;

  if((pid = fork()) == 0) //child
    execv(exec_argv[0], exec_argv);
  else if(pid == -1) //failed
    rb_raise(rb_eRuntimeError, "tmp_exec.exec %s", strerror(errno)); //need to change error type.
  else //Parent
  { int count = 0;
    while((error = wait(&status)) == -1 && errno == EINTR && count < 4) count++ ;
    if(error == -1)
      rb_raise(rb_eRuntimeError, "tmp_exec: %s", strerror(errno)); //need to change error type.
  }
  if(exec_argv != NULL)
          free(exec_argv);

  return INT2FIX(status);
}


void Init_tmpfile()
{
  // Create a Ruby class in this module.
  // rb_cObject is defined in ruby.h

  myModule = rb_define_module("TmpFileMod");
  myClass = rb_define_class_under(myModule, "TmpFile", rb_cObject);
  rb_define_alloc_func(myClass, tmp_alloc);

  rb_define_method(myClass, "initialize", tmp_initialize, -1); //2 args

  //Class methods
  rb_define_module_function(myClass, "open", tmp_open, -1); //2 args, but easier to code as optional.
	rb_define_module_function(myClass, "exec", tmp_exec, -1);

  //methods
  rb_define_method(myClass, "each_line", tmp_each_line, 0);
  rb_define_method(myClass, "gets", tmp_gets, 0);
  rb_define_method(myClass, "puts", tmp_puts, -1);
  rb_define_method(myClass, "print", tmp_print, -1);
  rb_define_method(myClass, "close", tmp_close, 0);
  rb_define_method(myClass, "unlink?", tmp_do_unlink, 0);
  rb_define_method(myClass, "no_unlink", tmp_set_no_unlink, 0);
  rb_define_method(myClass, "auto_unlink", tmp_set_unlink, 0);
  rb_define_method(myClass, "unlink", tmp_unlink, 0);
  rb_define_method(myClass, "rewind", tmp_rewind, -1); //optional argument for how far to rewind.
  rb_define_method(myClass, "seek_end", tmp_seek_end, 0);
  rb_define_method(myClass, "flush", tmp_flush, 0);
  rb_define_method(myClass, "to_s", tmp_to_s, 0);
}

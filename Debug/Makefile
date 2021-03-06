#################################
#version : v1.6
# author : luzxiang
#   date : 2018.07.13
#################################
TARGETS  := echo
SUFFIX   := $(strip cpp)#源文件的后缀
PROJDIR  := $(strip ..)#相对于Makefile的目录$(CURDIR) 放在Debug目录下
SRCDIRS  := $(addprefix $(PROJDIR)/src/,\
	    . log socket main fifo\
	    )#相对于Makefile的目录

LIBS     := -lpthread#-l库名
LDFLAGS  := -L. -L./lib  -Wl,-rpath=.,-rpath=./lib#指定编译时和运行时的库目录
INCFLAGS := $(addprefix -I,$(SRCDIRS)) #指定头文件目录 
#=========以下内容一般不需改动===========#
ver      := debug#默认为调试版本,make ver=release编译时将采用发布版本编译
CC       := g++ -std=c++11#
ifeq ($(ver),debug)
CFLAGS   := -c -O  -Werror -DDEBUG -g -Wall -Wpointer-arith -Wunreachable-code
else
CFLAGS   := -c -O3 -Werror -UDEBUG#
endif
OBJDIR   := $(strip ./objs)
BINDIR   := $(strip $(PROJDIR)/bin)
SOURCES  := $(foreach subdir, $(SRCDIRS), $(wildcard $(subdir)/*.$(SUFFIX)))
OBJECTS  := $(notdir $(SOURCES:.$(SUFFIX)=.o))#这里不要有路径
DEPENDS  := $(addprefix $(OBJDIR)/,$(notdir $(OBJECTS:.o=.d)))#这里要加路径,否则匹配不到规则

#创建bin目录
MKDIRBIN := $(shell if [ ! -d $(BINDIR) ]; then mkdir $(BINDIR); fi)
#创建obj目录
MKDIROBJ := $(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

VPATH := $(SRCDIRS):$(OBJDIR)
#vpath %.$(SUFFIX) $(SCDIRS)
#vpath %.o $(OBJDIR)
#vpath %.d $(OBJDIR)

.PHONY : all install print clean check  
all : $(TARGETS)

$(TARGETS) : $(OBJECTS)
	@echo 'Building target: $@'
	$(CC) -o $(@) $(addprefix $(OBJDIR)/,$(^F)) $(LDFLAGS) $(LIBS) 
	@echo 'Finished building target: $@'
	@echo ' '
	@chmod a+x $(@)
	@$(MAKE) install -s

%.o : %.$(SUFFIX)
	@echo 'Building file: $<'
	$(CC) -o $(OBJDIR)/$(@F) $< $(CFLAGS) $(INCFLAGS) 
	@echo 'Finished building file: $@'
	@echo ' '

$(OBJDIR)/%.d : %.$(SUFFIX)
	@set -e; rm -f $@; $(CC) $(INCFLAGS) -MM $< > $@.$$$$;\
	sed 's,\($*\)\.o[ : ]*,\1.o $@ : ,g' < $@.$$$$ > $@;
	@rm -f $@.*;

ifneq (clean,$(MAKECMDGOALS))
ifneq (check,$(MAKECMDGOALS))
ifneq (install,$(MAKECMDGOALS))
    sinclude $(DEPENDS) 
endif
endif
endif

#列出改变过的源文件
print:
	@echo '变更过的文件:'
	@find $(PROJDIR) -type f -name *.h -newer $(TARGETS)
	@find $(PROJDIR) -type f -name *.$(SUFFIX) -newer $(TARGETS)

#安装已编译好的程序，其实就是把目标可执行文件拷贝到指定的目录中去
install:
	@echo 'install...'
	@chmod a+x $(addprefix ./, $(notdir $(TARGETS)))
	@\cp -f $(addprefix ./, $(notdir $(TARGETS))) $(BINDIR)/

#删除所有被make创建的文件
clean: 
	$(RM) $(addprefix ./$(OBJDIR)/,*.d *.d.*)
	$(RM) $(addprefix ./$(OBJDIR)/,*.o *.o.*)
	$(RM) $(addprefix ./,$(notdir $(TARGETS)))
	$(RM) $(addprefix ./$(BINDIR)/,core.*)
	$(RM) $(addprefix ./,core.*)
check:
	@echo SRCDIRS:$(SRCDIRS)
	@echo SOURCES:$(SOURCES)
	@echo DEPENDS:$(DEPENDS)
	@echo OBJECTS:$(OBJECTS)
	@echo LDFLAGS:$(LDFLAGS)
	@echo CFLAGS:$(CFLAGS)
	@echo LIBS:$(LIBS)

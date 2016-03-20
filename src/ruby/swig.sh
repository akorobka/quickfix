#!/bin/sh

swig -I../C++ -DALIGN_DECL_DEFAULT -DHEAVYUSE -DPURE_DECL -ruby -c++ -o QuickfixRuby.cpp quickfix.i

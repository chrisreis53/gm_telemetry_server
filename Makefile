# Copyright 2007-2016 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.



SRCS   = gm_telemetry_server.cpp dataset.hpp
TARGET = gm_telemetry_server

include ../AppMake.mf

INCS += -I ./websocketpp -I/user/include/mysql

LDFLAGS += -lboost_system -lmysqlclient -lz

CXXFLAGS += -std=c++11

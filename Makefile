# Copyright 2007-2016 United States Government as represented by the
# Administrator of The National Aeronautics and Space Administration.
# No copyright is claimed in the United States under Title 17, U.S. Code.
# All Rights Reserved.



SRCS   = gm_telemetry_server.cpp
TARGET = gm_telemetry_server

include ../AppMake.mf

INCS += -I ./websocketpp

LDFLAGS += -lboost_system

CXXFLAGS += -std=c++11

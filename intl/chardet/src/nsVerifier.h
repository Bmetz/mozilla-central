/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#ifndef nsVerifier_h__
#define nsVerifier_h__

#include "nsPkgInt.h"

typedef enum {
   eStart = 0,
   eError = 1,
   eItsMe = 2 
} nsSMState;

typedef struct nsVerifier {
  const char* charset;
  nsPkgInt  cclass;
  PRUint32  stFactor; // >= number of cclass.
  nsPkgInt  states;
} nsVerifier;

#define GETCLASS(v,c) GETFROMPCK(((unsigned char)(c)), (v)->cclass)
#define GETNEXTSTATE(v,c,s) \
             GETFROMPCK((s)*((v)->stFactor)+GETCLASS((v),(c)), ((v)->states))

#endif /* nsVerifier_h__ */

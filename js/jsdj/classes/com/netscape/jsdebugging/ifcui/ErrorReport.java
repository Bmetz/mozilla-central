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

/*
* Holds error state information
*/

// when     who     what
// 06/30/97 jband   added this simple dumb data class
//

package com.netscape.jsdebugging.ifcui;

class ErrorReport
{
    private ErrorReport() {};    // 'Hidden' default ctor

    public  ErrorReport(
                String msg,
                String filename,
                int    lineno,
                String linebuf,
                int    tokenOffset )
    {
        this.msg = msg;
        this.filename = filename;
        this.lineno = lineno;
        this.linebuf = linebuf;
        this.tokenOffset = tokenOffset;
    }

    // data...
    public String msg;
    public String filename;
    public int    lineno;
    public String linebuf;
    public int    tokenOffset;
}

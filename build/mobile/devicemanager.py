# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.   
#
# The Original Code is Test Automation Framework.
#
# The Initial Developer of the Original Code is Joel Maher.
#
# Portions created by the Initial Developer are Copyright (C) 2009
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
# Joel Maher <joel.maher@gmail.com> (Original Developer)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

import socket
import time, datetime
import os
import re
import hashlib
import subprocess
from threading import Thread
import traceback
import sys

class FileError(Exception):
  " Signifies an error which occurs while doing a file operation."

  def __init__(self, msg = ''):
    self.msg = msg

  def __str__(self):
    return self.msg

class DeviceManager:
  host = ''
  port = 0
  debug = 3
  _redo = False
  deviceRoot = None
  tempRoot = os.getcwd()
  base_prompt = '\$\>'
  prompt_sep = '\x00'
  prompt_regex = '.*' + base_prompt + prompt_sep

  def __init__(self, host, port = 27020):
    self.host = host
    self.port = port
    self._sock = None
    self.getDeviceRoot()

  def sendCMD(self, cmdline, newline = True, sleep = 0):
    promptre = re.compile(self.prompt_regex + '$')

    # TODO: any commands that don't output anything and quit need to match this RE
    pushre = re.compile('^push .*$')
    data = ""
    noQuit = False

    if (self._sock == None):
      try:
        if (self.debug >= 1):
          print "reconnecting socket"
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      except:
        self._redo = True
        self._sock = None
        if (self.debug >= 2):
          print "unable to create socket"
        return None
      
      try:
        self._sock.connect((self.host, int(self.port)))
        self._sock.recv(1024)
      except:
        self._redo = True
        self._sock.close()
        self._sock = None
        if (self.debug >= 2):
          print "unable to connect socket"
        return None
      
    for cmd in cmdline:
      if (cmd == 'quit'): break
      if newline: cmd += '\r\n'
      
      try:
        self._sock.send(cmd)
        if (self.debug >= 4): print "send cmd: " + str(cmd)
      except:
        self._redo = True
        self._sock.close()
        self._sock = None
        return None
      
      if (pushre.match(cmd) or cmd == 'rebt'):
        noQuit = True
      elif noQuit == False:
        time.sleep(int(sleep))
        found = False
        while (found == False):
          if (self.debug >= 4): print "recv'ing..."
          
          try:
            temp = self._sock.recv(1024)
          except:
            self._redo = True
            self._sock.close()
            self._sock = None
            return None
          lines = temp.split('\n')
          for line in lines:
            if (promptre.match(line)):
              found = True
          data += temp

    time.sleep(int(sleep))
    if (noQuit == True):
      try:
        self._sock.close()
        self._sock = None
      except:
        self._redo = True
        self._sock = None
        return None

    return data
  
  
  # take a data blob and strip instances of the prompt '$>\x00'
  def stripPrompt(self, data):
    promptre = re.compile(self.prompt_regex + '.*')
    retVal = []
    lines = data.split('\n')
    for line in lines:
      try:
        while (promptre.match(line)):
          pieces = line.split(self.prompt_sep)
          index = pieces.index('$>')
          pieces.pop(index)
          line = self.prompt_sep.join(pieces)
      except(ValueError):
        pass
      retVal.append(line)

    return '\n'.join(retVal)
  

  def pushFile(self, localname, destname):
    if (self.debug >= 2): print "in push file with: " + localname + ", and: " + destname
    if (self.validateFile(destname, localname) == True):
      if (self.debug >= 2): print "files are validated"
      return ''

    if self.mkDirs(destname) == None:
      print "unable to make dirs: " + destname
      return None

    if (self.debug >= 2): print "sending: push " + destname
    
    # sleep 5 seconds / MB
    filesize = os.path.getsize(localname)
    sleepsize = 1024 * 1024
    sleepTime = (int(filesize / sleepsize) * 5) + 2
    f = open(localname, 'rb')
    data = f.read()
    f.close()
    retVal = self.sendCMD(['push ' + destname + '\r\n', data], newline = False, sleep = sleepTime)
    if (retVal == None):
      if (self.debug >= 2): print "Error in sendCMD, not validating push"
      return None

    if (self.validateFile(destname, localname) == False):
      if (self.debug >= 2): print "file did not copy as expected"
      return None

    return retVal
  
  def mkDir(self, name):
    return self.sendCMD(['mkdr ' + name])
  
  
  # make directory structure on the device
  def mkDirs(self, filename):
    parts = filename.split('/')
    name = ""
    for part in parts:
      if (part == parts[-1]): break
      if (part != ""):
        name += '/' + part
        if (self.mkDir(name) == None):
          print "failed making directory: " + str(name)
          return None
    return ''

  # push localDir from host to remoteDir on the device
  def pushDir(self, localDir, remoteDir):
    if (self.debug >= 2): print "pushing directory: " + localDir + " to " + remoteDir
    for root, dirs, files in os.walk(localDir):
      parts = root.split(localDir)
      for file in files:
        remoteRoot = remoteDir + '/' + parts[1]
        remoteName = remoteRoot + '/' + file
        if (parts[1] == ""): remoteRoot = remoteDir
        if (self.pushFile(os.path.join(root, file), remoteName) == None):
          time.sleep(5)
          self.removeFile(remoteName)
          time.sleep(5)
          if (self.pushFile(os.path.join(root, file), remoteName) == None):
            return None
    return True


  def dirExists(self, dirname):
    match = ".*" + dirname + "$"
    dirre = re.compile(match)
    data = self.sendCMD(['cd ' + dirname, 'cwd', 'quit'], sleep = 1)
    if (data == None):
      return None
    retVal = self.stripPrompt(data)
    data = retVal.split('\n')
    found = False
    for d in data:
      if (dirre.match(d)): 
        found = True

    return found

  # Because we always have / style paths we make this a lot easier with some
  # assumptions
  def fileExists(self, filepath):
    s = filepath.split('/')
    containingpath = '/'.join(s[:-1])
    listfiles = self.listFiles(containingpath)
    for f in listfiles:
      if (f == s[-1]):
        return True
    return False

  # list files on the device, requires cd to directory first
  def listFiles(self, rootdir):
    if (self.dirExists(rootdir) == False):
      return []  
    data = self.sendCMD(['cd ' + rootdir, 'ls', 'quit'], sleep=1)
    if (data == None):
      return None
    retVal = self.stripPrompt(data)
    return retVal.split('\n')

  def removeFile(self, filename):
    if (self.debug>= 2): print "removing file: " + filename
    return self.sendCMD(['rm ' + filename, 'quit'])
  
  
  # does a recursive delete of directory on the device: rm -Rf remoteDir
  def removeDir(self, remoteDir):
    self.sendCMD(['rmdr ' + remoteDir], sleep = 5)


  def getProcessList(self):
    data = self.sendCMD(['ps'], sleep = 3)
    if (data == None):
      return None
      
    retVal = self.stripPrompt(data)
    lines = retVal.split('\n')
    files = []
    for line in lines:
      if (line.strip() != ''):
        pidproc = line.strip().split()
        if (len(pidproc) == 2):
          files += [[pidproc[0], pidproc[1]]]
        elif (len(pidproc) == 3):
          #android returns <userID> <procID> <procName>
          files += [[pidproc[1], pidproc[2], pidproc[0]]]     
    return files

  def getMemInfo(self):
    data = self.sendCMD(['mems', 'quit'])
    if (data == None):
      return None
    retVal = self.stripPrompt(data)
    # TODO: this is hardcoded for now
    fhandle = open("memlog.txt", 'a')
    fhandle.write("\n")
    fhandle.write(retVal)
    fhandle.close()

  def fireProcess(self, appname):
    if (self.debug >= 2): print "FIRE PROC: '" + appname + "'"
    
    if (self.processExist(appname) != ''):
      print "WARNING: process %s appears to be running already\n" % appname
    
    self.sendCMD(['exec ' + appname])

    #NOTE: we sleep for 30 seconds to allow the application to startup
    time.sleep(30)

    self.process = self.processExist(appname)
    if (self.debug >= 4): print "got pid: " + str(self.process) + " for process: " + str(appname)

  def launchProcess(self, cmd, outputFile = "process.txt", cwd = ''):
    if (outputFile == "process.txt"):
      outputFile = self.getDeviceRoot() + '/' + "process.txt"

    cmdline = subprocess.list2cmdline(cmd)
    self.fireProcess(cmdline + " > " + outputFile)
    handle = outputFile

    return handle
  
  #hardcoded: sleep interval of 5 seconds, timeout of 10 minutes
  def communicate(self, process, timeout = 600):
    interval = 5
    timed_out = True
    if (timeout > 0):
      total_time = 0
      while total_time < timeout:
        time.sleep(interval)
        if (not self.poll(process)):
          timed_out = False
          break
        total_time += interval

    if (timed_out == True):
      return None

    return [self.getFile(process, "temp.txt"), None]


  def poll(self, process):
    try:
      if (self.processExist(process) == None):
        return None
      return 1
    except:
      return None
    return 1
  


  # iterates process list and returns pid if exists, otherwise ''
  def processExist(self, appname):
    pid = ''
  
    pieces = appname.split(' ')
    parts = pieces[0].split('/')
    app = parts[-1]
    procre = re.compile('.*' + app + '.*')

    procList = self.getProcessList()
    if (procList == None):
      return None
      
    for proc in procList:
      if (procre.match(proc[1])):
        pid = proc[0]
        break
    return pid


  def killProcess(self, appname):
    if (self.sendCMD(['kill ' + appname]) == None):
      return None

    return True

  def getTempDir(self):
    retVal = ''
    data = self.sendCMD(['tmpd', 'quit'])
    if (data == None):
      return None
    return self.stripPrompt(data).strip('\n')

  
  # copy file from device (remoteFile) to host (localFile)
  def getFile(self, remoteFile, localFile = ''):
    if localFile == '':
        localFile = os.path.join(self.tempRoot, "temp.txt")
  
    promptre = re.compile(self.prompt_regex + '.*')
    data = self.sendCMD(['cat ' + remoteFile, 'quit'], sleep = 5)
    if (data == None):
      return None
    retVal = self.stripPrompt(data)
    fhandle = open(localFile, 'wb')
    fhandle.write(retVal)
    fhandle.close()
    return retVal
  
  
  # copy directory structure from device (remoteDir) to host (localDir)
  def getDirectory(self, remoteDir, localDir):
    if (self.debug >= 2): print "getting files in '" + remoteDir + "'"
    filelist = self.listFiles(remoteDir)
    if (filelist == None):
      return None
    if (self.debug >= 3): print filelist
    if not os.path.exists(localDir):
      os.makedirs(localDir)
  
    # TODO: is this a comprehensive file regex?
    isFile = re.compile('^([a-zA-Z0-9_\-\. ]+)\.([a-zA-Z0-9]+)$')
    for f in filelist:
      if (isFile.match(f)):
        if (self.getFile(remoteDir + '/' + f, os.path.join(localDir, f)) == None):
          return None
      else:
        if (self.getDirectory(remoteDir + '/' + f, os.path.join(localDir, f)) == None):
          return None


  # true/false check if the two files have the same md5 sum
  def validateFile(self, remoteFile, localFile):
    remoteHash = self.getRemoteHash(remoteFile)
    localHash = self.getLocalHash(localFile)

    if (remoteHash == localHash):
        return True

    return False

  
  # return the md5 sum of a remote file
  def getRemoteHash(self, filename):
      data = self.sendCMD(['hash ' + filename, 'quit'], sleep = 1)
      if (data == None):
          return ''
      retVal = self.stripPrompt(data)
      if (retVal != None):
        retVal = retVal.strip('\n')
      if (self.debug >= 3): print "remote hash returned: '" + retVal + "'"
      return retVal
    

  # return the md5 sum of a file on the host
  def getLocalHash(self, filename):
      file = open(filename, 'rb')
      if (file == None):
          return None

      try:
        mdsum = hashlib.md5()
      except:
        return None

      while 1:
          data = file.read(1024)
          if not data:
              break
          mdsum.update(data)

      file.close()
      hexval = mdsum.hexdigest()
      if (self.debug >= 3): print "local hash returned: '" + hexval + "'"
      return hexval

  # Gets the device root for the testing area on the device
  # For all devices we will use / type slashes and depend on the device-agent
  # to sort those out.  The agent will return us the device location where we
  # should store things, we will then create our /tests structure relative to
  # that returned path.
  # Structure on the device is as follows:
  # /tests
  #       /<fennec>|<firefox>  --> approot
  #       /profile
  #       /xpcshell
  #       /reftest
  #       /mochitest
  def getDeviceRoot(self):
    if (not self.deviceRoot):
      data = self.sendCMD(['testroot'], sleep = 1)
      if (data == None):
        return '/tests'
      self.deviceRoot = self.stripPrompt(data).strip('\n') + '/tests'

    if (not self.dirExists(self.deviceRoot)):
      self.mkDir(self.deviceRoot)

    return self.deviceRoot

  # Either we will have /tests/fennec or /tests/firefox but we will never have
  # both.  Return the one that exists
  def getAppRoot(self):
    if (self.dirExists(self.getDeviceRoot() + '/fennec')):
      return self.getDeviceRoot() + '/fennec'
    elif (self.dirExists(self.getDeviceRoot() + '/firefox')):
      return self.getDeviceRoot() + '/firefox'
    else:
      return 'org.mozilla.fennec'

  # Gets the directory location on the device for a specific test type
  # Type is one of: xpcshell|reftest|mochitest
  def getTestRoot(self, type):
    if (re.search('xpcshell', type, re.I)):
      self.testRoot = self.getDeviceRoot() + '/xpcshell'
    elif (re.search('?(i)reftest', type)):
      self.testRoot = self.getDeviceRoot() + '/reftest'
    elif (re.search('?(i)mochitest', type)):
      self.testRoot = self.getDeviceRoot() + '/mochitest'
    return self.testRoot

  # Sends a specific process ID a signal code and action.
  # For Example: SIGINT and SIGDFL to process x
  def signal(self, processID, signalType, signalAction):
    # currently not implemented in device agent - todo
    pass

  # Get a return code from process ending -- needs support on device-agent
  # this is a todo
  def getReturnCode(self, processID):
    # todo make this real
    return 0

  def unpackFile(self, filename):
    dir = ''
    parts = filename.split('/')
    if (len(parts) > 1):
      if self.fileExists(filename):
        dir = '/'.join(parts[:-1])
    elif self.fileExists('/' + filename):
      dir = '/' + filename
    elif self.fileExists(self.getDeviceRoot() + '/' + filename):
      dir = self.getDeviceRoot() + '/' + filename
    else:
      return None

    return self.sendCMD(['cd ' + dir, 'unzp ' + filename])


  def reboot(self, wait = False):
    self.sendCMD(['rebt'])

    if wait == True:
      time.sleep(30)
      timeout = 270
      done = False
      while (not done):
        if self.listFiles('/') != None:
          return ''
        print "sleeping another 10 seconds"
        time.sleep(10)
        timeout = timeout - 10
        if (timeout <= 0):
          return None
    return ''

  # validate localDir from host to remoteDir on the device
  def validateDir(self, localDir, remoteDir):
    if (self.debug >= 2): print "validating directory: " + localDir + " to " + remoteDir
    for root, dirs, files in os.walk(localDir):
      parts = root.split(localDir)
      for file in files:
        remoteRoot = remoteDir + '/' + parts[1]
        remoteRoot = remoteRoot.replace('/', '/')
        if (parts[1] == ""): remoteRoot = remoteDir
        remoteName = remoteRoot + '/' + file
        if (self.validateFile(remoteName, os.path.join(root, file)) <> True):
            return None
    return True

  #TODO: make this simpler by doing a single directive at a time
  # Returns information about the device:
  # Directive indicates the information you want to get, your choices are:
  # os - name of the os
  # id - unique id of the device
  # uptime - uptime of the device
  # systime - system time of the device
  # screen - screen resolution
  # memory - memory stats
  # process - list of running processes (same as ps)
  # disk - total, free, available bytes on disk
  # power - power status (charge, battery temp)
  # all - all of them
  def getInfo(self, directive):
    data = None
    if (directive in ('os','id','uptime','systime','screen','memory','process',
                      'disk','power')):
      data = self.sendCMD(['info ' + directive, 'quit'], sleep = 1)
    else:
      directive = None
      data = self.sendCMD(['info', 'quit'], sleep = 1)

    if (data is None):
      return None
      
    data = self.stripPrompt(data)
    result = {}
        
    if directive:
      result[directive] = data.split('\n')
      for i in range(len(result[directive])):
        if (len(result[directive][i]) != 0):
          result[directive][i] = result[directive][i].strip()

      # Get rid of any empty attributes
      result[directive].remove('')

    else:
      lines = data.split('\n')
      result['id'] = lines[0]
      result['os'] = lines[1]
      result['systime'] = lines[2]
      result['uptime'] = lines[3]
      result['screen'] = lines[4]
      result['memory'] = lines[5]
      if (lines[6] == 'Power status'):
        tmp = []
        for i in range(4):
          tmp.append(line[7 + i])
        result['power'] = tmp
      tmp = []

      # Linenum is the line where the process list begins
      linenum = 11
      for j in range(len(lines) - linenum):
        if (lines[j + linenum].strip() != ''):
          procline = lines[j + linenum].split('\t')

          if len(procline) == 2:
            tmp.append([procline[0], procline[1]])
          elif len(procline) == 3:
            # Android has <userid> <procid> <procname>
            # We put the userid to achieve a common format
            tmp.append([procline[1], procline[2], procline[0]])
      result['process'] = tmp
    return result

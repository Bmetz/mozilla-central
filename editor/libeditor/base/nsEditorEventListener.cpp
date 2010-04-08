/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=4 sw=2 et tw=78: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Masayuki Nakano <masayuki@d-toybox.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include "nsEditorEventListener.h"
#include "nsEditor.h"
#include "nsIPlaintextEditor.h"

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIPrivateTextEvent.h"
#include "nsIPrivateCompositionEvent.h"
#include "nsIEditorMailSupport.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsILookAndFeel.h"
#include "nsPresContext.h"
#include "nsFocusManager.h"

// Drag & Drop, Clipboard
#include "nsIServiceManager.h"
#include "nsIClipboard.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsIContent.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMNSRange.h"
#include "nsEditorUtils.h"
#include "nsIDOMEventTarget.h"
#include "nsIEventStateManager.h"
#include "nsISelectionPrivate.h"
#include "nsIDOMDragEvent.h"
#include "nsIFocusManager.h"
#include "nsIDOMWindow.h"

nsEditorEventListener::nsEditorEventListener(nsEditor* aEditor) :
  mEditor(aEditor), mCaretDrawn(PR_FALSE), mCommitText(PR_FALSE),
  mInTransaction(PR_FALSE)
{
}

nsEditorEventListener::~nsEditorEventListener() 
{
}

already_AddRefed<nsIPresShell>
nsEditorEventListener::GetPresShell()
{
  NS_ENSURE_TRUE(mEditor, nsnull);
  // mEditor is nsEditor or its inherited class.
  // This is guaranteed by constructor.
  nsCOMPtr<nsIPresShell> ps;
  static_cast<nsEditor*>(mEditor)->GetPresShell(getter_AddRefs(ps));
  return ps.forget();
}

/**
 *  nsISupports implementation
 */

NS_IMPL_ADDREF(nsEditorEventListener)
NS_IMPL_RELEASE(nsEditorEventListener)

NS_INTERFACE_MAP_BEGIN(nsEditorEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTextListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCompositionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMKeyListener)
NS_INTERFACE_MAP_END

/**
 *  nsIDOMEventListener implementation
 */

NS_IMETHODIMP
nsEditorEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMDragEvent> dragEvent = do_QueryInterface(aEvent);
  if (dragEvent) {
    nsAutoString eventType;
    aEvent->GetType(eventType);
    if (eventType.EqualsLiteral("draggesture"))
      return DragGesture(dragEvent);
    if (eventType.EqualsLiteral("dragenter"))
      return DragEnter(dragEvent);
    if (eventType.EqualsLiteral("dragover"))
      return DragOver(dragEvent);
    if (eventType.EqualsLiteral("dragleave"))
      return DragLeave(dragEvent);
    if (eventType.EqualsLiteral("drop"))
      return Drop(dragEvent);
  }
  return NS_OK;
}

/**
 * nsIDOMKeyListener implementation
 */

NS_IMETHODIMP
nsEditorEventListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  // DOM event handling happens in two passes, the client pass and the system
  // pass.  We do all of our processing in the system pass, to allow client
  // handlers the opportunity to cancel events and prevent typing in the editor.
  // If the client pass cancelled the event, defaultPrevented will be true
  // below.

  nsCOMPtr<nsIDOMNSUIEvent> nsUIEvent = do_QueryInterface(aKeyEvent);
  if(nsUIEvent) 
  {
    PRBool defaultPrevented;
    nsUIEvent->GetPreventDefault(&defaultPrevented);
    if(defaultPrevented)
      return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent>keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) 
  {
    //non-key event passed to keypress.  bad things.
    return NS_OK;
  }

  // we should check a flag here to see if we should be using built-in key bindings
  // mEditor->GetFlags(&flags);
  // if (flags & ...)

  PRUint32 keyCode;
  keyEvent->GetKeyCode(&keyCode);

  // if we are readonly or disabled, then do nothing.
  PRUint32 flags;
  if (NS_SUCCEEDED(mEditor->GetFlags(&flags)))
  {
    if (flags & nsIPlaintextEditor::eEditorReadonlyMask || 
        flags & nsIPlaintextEditor::eEditorDisabledMask) 
    {
      // consume backspace for disabled and readonly textfields, to prevent
      // back in history, which could be confusing to users
      if (keyCode == nsIDOMKeyEvent::DOM_VK_BACK_SPACE)
        aKeyEvent->PreventDefault();

      return NS_OK;
    }
  }
  else
    return NS_ERROR_FAILURE;  // Editor unable to handle this.

  nsCOMPtr<nsIPlaintextEditor> textEditor (do_QueryInterface(mEditor));
  if (!textEditor) return NS_ERROR_NO_INTERFACE;

  // if there is no charCode, then it's a key that doesn't map to a character,
  // so look for special keys using keyCode.
  if (0 != keyCode)
  {
    PRBool isAnyModifierKeyButShift;
    nsresult rv;
    rv = keyEvent->GetAltKey(&isAnyModifierKeyButShift);
    if (NS_FAILED(rv)) return rv;
    
    if (!isAnyModifierKeyButShift)
    {
      rv = keyEvent->GetMetaKey(&isAnyModifierKeyButShift);
      if (NS_FAILED(rv)) return rv;
      
      if (!isAnyModifierKeyButShift)
      {
        rv = keyEvent->GetCtrlKey(&isAnyModifierKeyButShift);
        if (NS_FAILED(rv)) return rv;
      }
    }

    switch (keyCode)
    {
      case nsIDOMKeyEvent::DOM_VK_META:
      case nsIDOMKeyEvent::DOM_VK_SHIFT:
      case nsIDOMKeyEvent::DOM_VK_CONTROL:
      case nsIDOMKeyEvent::DOM_VK_ALT:
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK;
        break;

      case nsIDOMKeyEvent::DOM_VK_BACK_SPACE: 
        if (isAnyModifierKeyButShift)
          return NS_OK;

        mEditor->DeleteSelection(nsIEditor::ePrevious);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK;
        break;
 
      case nsIDOMKeyEvent::DOM_VK_DELETE:
        /* on certain platforms (such as windows) the shift key
           modifies what delete does (cmd_cut in this case).
           bailing here to allow the keybindings to do the cut.*/
        PRBool isShiftModifierKey;
        rv = keyEvent->GetShiftKey(&isShiftModifierKey);
        if (NS_FAILED(rv)) return rv;

        if (isAnyModifierKeyButShift || isShiftModifierKey)
           return NS_OK;
        mEditor->DeleteSelection(nsIEditor::eNext);
        aKeyEvent->PreventDefault(); // consumed
        return NS_OK; 
        break;
 
      case nsIDOMKeyEvent::DOM_VK_TAB:
        if ((flags & nsIPlaintextEditor::eEditorSingleLineMask) ||
            (flags & nsIPlaintextEditor::eEditorPasswordMask)   ||
            (flags & nsIPlaintextEditor::eEditorWidgetMask)     ||
            (flags & nsIPlaintextEditor::eEditorAllowInteraction))
          return NS_OK; // let it be used for focus switching

        if (isAnyModifierKeyButShift)
          return NS_OK;

        // else we insert the tab straight through
        textEditor->HandleKeyPress(keyEvent);
        // let HandleKeyPress consume the event
        return NS_OK; 

      case nsIDOMKeyEvent::DOM_VK_RETURN:
      case nsIDOMKeyEvent::DOM_VK_ENTER:
        if (isAnyModifierKeyButShift)
          return NS_OK;

        if (!(flags & nsIPlaintextEditor::eEditorSingleLineMask))
        {
          textEditor->HandleKeyPress(keyEvent);
          aKeyEvent->PreventDefault(); // consumed
        }
        return NS_OK;
    }
  }

  textEditor->HandleKeyPress(keyEvent);
  return NS_OK; // we don't PreventDefault() here or keybindings like control-x won't work 
}

/**
 * nsIDOMMouseListener implementation
 */

NS_IMETHODIMP
nsEditorEventListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  nsCOMPtr<nsIDOMNSEvent> nsevent = do_QueryInterface(aMouseEvent);
  PRBool isTrusted = PR_FALSE;
  if (!mouseEvent || !nsevent ||
      NS_FAILED(nsevent->GetIsTrusted(&isTrusted)) || !isTrusted) {
    // Non-ui or non-trusted event passed in. Bad things.
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aMouseEvent);
  if (!nsuiEvent)
    return NS_ERROR_NULL_POINTER;

  PRBool preventDefault;
  rv = nsuiEvent->GetPreventDefault(&preventDefault);
  if (NS_FAILED(rv) || preventDefault)
  {
    // We're done if 'preventdefault' is true (see for example bug 70698).
    return rv;
  }

  nsCOMPtr<nsIEditor> editor = do_QueryInterface(mEditor);
  if (!editor) { return NS_OK; }

  // If we got a mouse down inside the editing area, we should force the 
  // IME to commit before we change the cursor position
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  if (imeEditor)
    imeEditor->ForceCompositionEnd();

  PRUint16 button = (PRUint16)-1;
  mouseEvent->GetButton(&button);
  // middle-mouse click (paste);
  if (button == 1)
  {
    nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && prefBranch)
    {
      PRBool doMiddleMousePaste = PR_FALSE;;
      rv = prefBranch->GetBoolPref("middlemouse.paste", &doMiddleMousePaste);
      if (NS_SUCCEEDED(rv) && doMiddleMousePaste)
      {
        // Set the selection to the point under the mouse cursor:
        nsCOMPtr<nsIDOMNode> parent;
        if (NS_FAILED(nsuiEvent->GetRangeParent(getter_AddRefs(parent))))
          return NS_ERROR_NULL_POINTER;
        PRInt32 offset = 0;
        if (NS_FAILED(nsuiEvent->GetRangeOffset(&offset)))
          return NS_ERROR_NULL_POINTER;

        nsCOMPtr<nsISelection> selection;
        if (NS_SUCCEEDED(editor->GetSelection(getter_AddRefs(selection))))
          (void)selection->Collapse(parent, offset);

        // If the ctrl key is pressed, we'll do paste as quotation.
        // Would've used the alt key, but the kde wmgr treats alt-middle specially. 
        PRBool ctrlKey = PR_FALSE;
        mouseEvent->GetCtrlKey(&ctrlKey);

        nsCOMPtr<nsIEditorMailSupport> mailEditor;
        if (ctrlKey)
          mailEditor = do_QueryInterface(mEditor);

        PRInt32 clipboard;

#if defined(XP_OS2) || defined(XP_WIN32)
        clipboard = nsIClipboard::kGlobalClipboard;
#else
        clipboard = nsIClipboard::kSelectionClipboard;
#endif

        if (mailEditor)
          mailEditor->PasteAsQuotation(clipboard);
        else
          editor->Paste(clipboard);

        // Prevent the event from propagating up to be possibly handled
        // again by the containing window:
        mouseEvent->StopPropagation();
        mouseEvent->PreventDefault();

        // We processed the event, whether drop/paste succeeded or not
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  if (!imeEditor)
    return NS_OK;
  imeEditor->ForceCompositionEnd();
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK;
}

/**
 * nsIDOMTextListener implementation
 */

NS_IMETHODIMP
nsEditorEventListener::HandleText(nsIDOMEvent* aTextEvent)
{
   nsCOMPtr<nsIPrivateTextEvent> textEvent = do_QueryInterface(aTextEvent);
   if (!textEvent) {
      //non-ui event passed in.  bad things.
      return NS_OK;
   }

   nsAutoString                      composedText;
   nsresult                          result;
   nsCOMPtr<nsIPrivateTextRangeList> textRangeList;
   nsTextEventReply*                 textEventReply;

   textEvent->GetText(composedText);
   textRangeList = textEvent->GetInputRange();
   textEventReply = textEvent->GetEventReply();
   nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor, &result);
   if (imeEditor) {
     PRUint32 flags;
     // if we are readonly or disabled, then do nothing.
     if (NS_SUCCEEDED(mEditor->GetFlags(&flags))) {
       if (flags & nsIPlaintextEditor::eEditorReadonlyMask || 
           flags & nsIPlaintextEditor::eEditorDisabledMask) {
         return NS_OK;
       }
     }
     result = imeEditor->SetCompositionString(composedText,textRangeList,textEventReply);
   }
   return result;
}

/**
 * Drag event implementation
 */

nsresult
nsEditorEventListener::DragGesture(nsIDOMDragEvent* aDragEvent)
{
  if ( !mEditor )
    return NS_ERROR_NULL_POINTER;

  // ...figure out if a drag should be started...
  PRBool canDrag;
  nsresult rv = mEditor->CanDrag(aDragEvent, &canDrag);
  if ( NS_SUCCEEDED(rv) && canDrag )
    rv = mEditor->DoDrag(aDragEvent);

  return rv;
}

nsresult
nsEditorEventListener::DragEnter(nsIDOMDragEvent* aDragEvent)
{
  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!presShell)
    return NS_OK;

  if (!mCaret)
  {
    NS_NewCaret(getter_AddRefs(mCaret));
    if (mCaret)
    {
      mCaret->Init(presShell);
      mCaret->SetCaretReadOnly(PR_TRUE);
    }
    mCaretDrawn = PR_FALSE;
  }

  presShell->SetCaret(mCaret);

  return DragOver(aDragEvent);
}

nsresult
nsEditorEventListener::DragOver(nsIDOMDragEvent* aDragEvent)
{
  // XXX cache this between drag events?
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (!dragService) return rv;

  // does the drag have flavors we can accept?
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> parent;
  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aDragEvent);
  if (nsuiEvent) {
    nsuiEvent->GetRangeParent(getter_AddRefs(parent));
    nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
    if (!dropParent)
      return NS_ERROR_FAILURE;

    if (!dropParent->IsEditable())
      return NS_OK;
  }

  PRBool canDrop = CanDrop(aDragEvent);
  dragSession->SetCanDrop(canDrop);

  if (canDrop)
  {
    // We need to consume the event to prevent the browser's
    // default drag listeners from being fired. (Bug 199133)
    aDragEvent->PreventDefault(); // consumed

    if (mCaret && nsuiEvent)
    {
      PRInt32 offset = 0;
      rv = nsuiEvent->GetRangeOffset(&offset);
      if (NS_FAILED(rv)) return rv;

      // to avoid flicker, we could track the node and offset to see if we moved
      if (mCaretDrawn)
        mCaret->EraseCaret();
      
      //mCaret->SetCaretVisible(PR_TRUE);   // make sure it's visible
      mCaret->DrawAtPosition(parent, offset);
      mCaretDrawn = PR_TRUE;
    }
  }
  else
  {
    if (mCaret && mCaretDrawn)
    {
      mCaret->EraseCaret();
      mCaretDrawn = PR_FALSE;
    } 
  }

  return NS_OK;
}

nsresult
nsEditorEventListener::DragLeave(nsIDOMDragEvent* aDragEvent)
{
  if (mCaret && mCaretDrawn)
  {
    mCaret->EraseCaret();
    mCaretDrawn = PR_FALSE;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (presShell)
    presShell->RestoreCaret();

  return NS_OK;
}

nsresult
nsEditorEventListener::Drop(nsIDOMDragEvent* aMouseEvent)
{
  if (mCaret)
  {
    if (mCaretDrawn)
    {
      mCaret->EraseCaret();
      mCaretDrawn = PR_FALSE;
    }
    mCaret->SetCaretVisible(PR_FALSE);    // hide it, so that it turns off its timer

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (presShell)
    {
      presShell->RestoreCaret();
    }
  }

  if (!mEditor)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent = do_QueryInterface(aMouseEvent);
  if (nsuiEvent) {
    nsCOMPtr<nsIDOMNode> parent;
    nsuiEvent->GetRangeParent(getter_AddRefs(parent));
    nsCOMPtr<nsIContent> dropParent = do_QueryInterface(parent);
    if (!dropParent)
      return NS_ERROR_FAILURE;

    if (!dropParent->IsEditable())
      return NS_OK;
  }

  PRBool canDrop = CanDrop(aMouseEvent);
  if (!canDrop)
  {
    // was it because we're read-only?

    PRUint32 flags;
    if (NS_SUCCEEDED(mEditor->GetFlags(&flags))
        && ((flags & nsIPlaintextEditor::eEditorDisabledMask) ||
            (flags & nsIPlaintextEditor::eEditorReadonlyMask)) )
    {
      // it was decided to "eat" the event as this is the "least surprise"
      // since someone else handling it might be unintentional and the 
      // user could probably re-drag to be not over the disabled/readonly 
      // editfields if that is what is desired.
      return aMouseEvent->StopPropagation();
    }
    return NS_OK;
  }

  aMouseEvent->StopPropagation();
  aMouseEvent->PreventDefault();
  // Beware! This may flush notifications via synchronous
  // ScrollSelectionIntoView.
  return mEditor->InsertFromDrop(aMouseEvent);
}

PRBool
nsEditorEventListener::CanDrop(nsIDOMDragEvent* aEvent)
{
  // if the target doc is read-only, we can't drop
  PRUint32 flags;
  if (NS_FAILED(mEditor->GetFlags(&flags)))
    return PR_FALSE;

  if ((flags & nsIPlaintextEditor::eEditorDisabledMask) || 
      (flags & nsIPlaintextEditor::eEditorReadonlyMask)) {
    return PR_FALSE;
  }

  // XXX cache this between drag events?
  nsresult rv;
  nsCOMPtr<nsIDragService> dragService = do_GetService("@mozilla.org/widget/dragservice;1", &rv);

  // does the drag have flavors we can accept?
  nsCOMPtr<nsIDragSession> dragSession;
  if (dragService)
    dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (!dragSession) return PR_FALSE;

  PRBool flavorSupported = PR_FALSE;
  dragSession->IsDataFlavorSupported(kUnicodeMime, &flavorSupported);

  if (!flavorSupported)
    dragSession->IsDataFlavorSupported(kMozTextInternal, &flavorSupported);

  // if we aren't plaintext editing, we can accept more flavors
  if (!flavorSupported 
     && (flags & nsIPlaintextEditor::eEditorPlaintextMask) == 0)
  {
    dragSession->IsDataFlavorSupported(kHTMLMime, &flavorSupported);
    if (!flavorSupported)
      dragSession->IsDataFlavorSupported(kFileMime, &flavorSupported);
#if 0
    if (!flavorSupported)
      dragSession->IsDataFlavorSupported(kJPEGImageMime, &flavorSupported);
#endif
  }

  if (!flavorSupported)
    return PR_FALSE;     

  nsCOMPtr<nsIDOMDocument> domdoc;
  rv = mEditor->GetDocument(getter_AddRefs(domdoc));
  if (NS_FAILED(rv)) return PR_FALSE;

  nsCOMPtr<nsIDOMDocument> sourceDoc;
  rv = dragSession->GetSourceDocument(getter_AddRefs(sourceDoc));
  if (NS_FAILED(rv)) return PR_FALSE;
  if (domdoc == sourceDoc)      // source and dest are the same document; disallow drops within the selection
  {
    nsCOMPtr<nsISelection> selection;
    rv = mEditor->GetSelection(getter_AddRefs(selection));
    if (NS_FAILED(rv) || !selection)
      return PR_FALSE;
    
    PRBool isCollapsed;
    rv = selection->GetIsCollapsed(&isCollapsed);
    if (NS_FAILED(rv)) return PR_FALSE;
  
    // Don't bother if collapsed - can always drop
    if (!isCollapsed)
    {
      nsCOMPtr<nsIDOMNSUIEvent> nsuiEvent (do_QueryInterface(aEvent));
      if (!nsuiEvent) return PR_FALSE;

      nsCOMPtr<nsIDOMNode> parent;
      rv = nsuiEvent->GetRangeParent(getter_AddRefs(parent));
      if (NS_FAILED(rv) || !parent) return PR_FALSE;

      PRInt32 offset = 0;
      rv = nsuiEvent->GetRangeOffset(&offset);
      if (NS_FAILED(rv)) return PR_FALSE;

      PRInt32 rangeCount;
      rv = selection->GetRangeCount(&rangeCount);
      if (NS_FAILED(rv)) return PR_FALSE;

      for (PRInt32 i = 0; i < rangeCount; i++)
      {
        nsCOMPtr<nsIDOMRange> range;
        rv = selection->GetRangeAt(i, getter_AddRefs(range));
        nsCOMPtr<nsIDOMNSRange> nsrange(do_QueryInterface(range));
        if (NS_FAILED(rv) || !nsrange) 
          continue; //don't bail yet, iterate through them all

        PRBool inRange = PR_TRUE;
        (void)nsrange->IsPointInRange(parent, offset, &inRange);
        if (inRange)
          return PR_FALSE;  //okay, now you can bail, we are over the orginal selection
      }
    }
  }
  
  return PR_TRUE;
}

/**
 * nsIDOMCompositionListener implementation
 */

NS_IMETHODIMP
nsEditorEventListener::HandleStartComposition(nsIDOMEvent* aCompositionEvent)
{
  nsCOMPtr<nsIPrivateCompositionEvent> pCompositionEvent = do_QueryInterface(aCompositionEvent);
  if (!pCompositionEvent) return NS_ERROR_FAILURE;

  nsTextEventReply* eventReply;
  nsresult rv = pCompositionEvent->GetCompositionReply(&eventReply);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  NS_ASSERTION(imeEditor, "The editor doesn't support IME?");
  return imeEditor->BeginComposition(eventReply);
}

NS_IMETHODIMP
nsEditorEventListener::HandleEndComposition(nsIDOMEvent* aCompositionEvent)
{
  nsCOMPtr<nsIEditorIMESupport> imeEditor = do_QueryInterface(mEditor);
  NS_ASSERTION(imeEditor, "The editor doesn't support IME?");
  return imeEditor->EndComposition();
}

/**
 * nsIDOMFocusListener implementation
 */

static already_AddRefed<nsIContent>
FindSelectionRoot(nsIEditor *aEditor, nsIContent *aContent)
{
  PRUint32 flags;
  aEditor->GetFlags(&flags);

  nsIDocument *document = aContent->GetCurrentDoc();
  if (!document) {
    return nsnull;
  }

  nsIContent *root;
  if (document->HasFlag(NODE_IS_EDITABLE)) {
    NS_IF_ADDREF(root = document->GetRootContent());

    return root;
  }

  if (flags & nsIPlaintextEditor::eEditorReadonlyMask) {
    // We still want to allow selection in a readonly editor.
    nsCOMPtr<nsIDOMElement> rootElement;
    aEditor->GetRootElement(getter_AddRefs(rootElement));
    if (!rootElement) {
      return nsnull;
    }

    CallQueryInterface(rootElement, &root);

    return root;
  }

  if (!aContent->HasFlag(NODE_IS_EDITABLE)) {
    return nsnull;
  }

  // For non-readonly editors we want to find the root of the editable subtree
  // containing aContent.
  nsIContent *parent, *content = aContent;
  while ((parent = content->GetParent()) && parent->HasFlag(NODE_IS_EDITABLE)) {
    content = parent;
  }

  NS_IF_ADDREF(content);

  return content;
}

NS_IMETHODIMP
nsEditorEventListener::Focus(nsIDOMEvent* aEvent)
{
  NS_ENSURE_ARG(aEvent);

  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetTarget(getter_AddRefs(target));

  // turn on selection and caret
  if (mEditor)
  {
    PRUint32 flags;
    mEditor->GetFlags(&flags);
    if (! (flags & nsIPlaintextEditor::eEditorDisabledMask))
    { // only enable caret and selection if the editor is not disabled
      nsCOMPtr<nsIContent> content = do_QueryInterface(target);

      PRBool targetIsEditableDoc = PR_FALSE;
      nsCOMPtr<nsIContent> editableRoot;
      if (content) {
        editableRoot = FindSelectionRoot(mEditor, content);

        // make sure that the element is really focused in case an earlier
        // listener in the chain changed the focus.
        if (editableRoot) {
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          NS_ENSURE_TRUE(fm, NS_OK);

          nsCOMPtr<nsIDOMElement> element;
          fm->GetFocusedElement(getter_AddRefs(element));
          if (!SameCOMIdentity(element, target))
            return NS_OK;
        }
      }
      else {
        nsCOMPtr<nsIDocument> document = do_QueryInterface(target);
        targetIsEditableDoc = document && document->HasFlag(NODE_IS_EDITABLE);
      }

      nsCOMPtr<nsISelectionController> selCon;
      mEditor->GetSelectionController(getter_AddRefs(selCon));
      if (selCon && (targetIsEditableDoc || editableRoot))
      {
        nsCOMPtr<nsISelection> selection;
        selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                             getter_AddRefs(selection));

        nsCOMPtr<nsIPresShell> presShell = GetPresShell();
        if (presShell) {
          nsRefPtr<nsCaret> caret = presShell->GetCaret();
          if (caret) {
            caret->SetIgnoreUserModify(PR_FALSE);
            if (selection) {
              caret->SetCaretDOMSelection(selection);
            }
          }
        }

        const PRBool kIsReadonly = (flags & nsIPlaintextEditor::eEditorReadonlyMask) != 0;
        selCon->SetCaretReadOnly(kIsReadonly);
        selCon->SetCaretEnabled(PR_TRUE);
        selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
        selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);

        nsCOMPtr<nsISelectionPrivate> selectionPrivate =
          do_QueryInterface(selection);
        if (selectionPrivate)
        {
          selectionPrivate->SetAncestorLimiter(editableRoot);
        }

        if (selection && !editableRoot) {
          PRInt32 rangeCount;
          selection->GetRangeCount(&rangeCount);
          if (rangeCount == 0) {
            mEditor->BeginningOfDocument();
          }
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEditorEventListener::Blur(nsIDOMEvent* aEvent)
{
  // check if something else is focused. If another element is focused, then
  // we should not change the selection.
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  NS_ENSURE_TRUE(fm, NS_OK);

  nsCOMPtr<nsIDOMElement> element;
  fm->GetFocusedElement(getter_AddRefs(element));
  if (element)
    return NS_OK;

  NS_ENSURE_ARG(aEvent);
  // turn off selection and caret
  if (mEditor)
  {
    nsCOMPtr<nsIEditor>editor = do_QueryInterface(mEditor);
    if (editor)
    {
      nsCOMPtr<nsISelectionController>selCon;
      editor->GetSelectionController(getter_AddRefs(selCon));
      if (selCon)
      {
        nsCOMPtr<nsISelection> selection;
        selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                             getter_AddRefs(selection));

        nsCOMPtr<nsISelectionPrivate> selectionPrivate =
          do_QueryInterface(selection);
        if (selectionPrivate) {
          selectionPrivate->SetAncestorLimiter(nsnull);
        }

        nsCOMPtr<nsIPresShell> presShell = GetPresShell();
        if (presShell) {
          nsRefPtr<nsCaret> caret = presShell->GetCaret();
          if (caret) {
            caret->SetIgnoreUserModify(PR_TRUE);
          }
        }

        selCon->SetCaretEnabled(PR_FALSE);

        PRUint32 flags;
        mEditor->GetFlags(&flags);
        if((flags & nsIPlaintextEditor::eEditorWidgetMask)  ||
          (flags & nsIPlaintextEditor::eEditorPasswordMask) ||
          (flags & nsIPlaintextEditor::eEditorReadonlyMask) ||
          (flags & nsIPlaintextEditor::eEditorDisabledMask) ||
          (flags & nsIPlaintextEditor::eEditorFilterInputMask))
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);//hide but do NOT turn off
        }
        else
        {
          selCon->SetDisplaySelection(nsISelectionController::SELECTION_DISABLED);
        }

        selCon->RepaintSelection(nsISelectionController::SELECTION_NORMAL);
      }
    }
  }

  return NS_OK;
}

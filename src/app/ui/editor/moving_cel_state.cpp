/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_cel_state.h"

#include "app/app.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/message.h"
#include "app/ui_context.h"
#include "app/undo_transaction.h"

namespace app {

using namespace ui;

MovingCelState::MovingCelState(Editor* editor, MouseMessage* msg)
  : m_canceled(false)
{
  Document* document = editor->getDocument();
  Sprite* sprite = editor->getSprite();
  LayerImage* layer = static_cast<LayerImage*>(editor->getLayer());
  ASSERT(layer->isImage());

  m_cel = layer->getCel(editor->getFrame());
  if (m_cel) {
    m_celStartX = m_cel->getX();
    m_celStartY = m_cel->getY();
  }
  else {
    m_celStartX = 0;
    m_celStartY = 0;
  }
  m_celNewX = m_celStartX;
  m_celNewY = m_celStartY;

  editor->screenToEditor(msg->position().x, msg->position().y, &m_mouseStartX, &m_mouseStartY);
  editor->captureMouse();

  // Hide the mask (temporarily, until mouse-up event)
  m_maskVisible = document->isMaskVisible();
  if (m_maskVisible) {
    document->setMaskVisible(false);
    document->generateMaskBoundaries();
  }
}

MovingCelState::~MovingCelState()
{
}

bool MovingCelState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  Document* document = editor->getDocument();

  // Here we put back the cel into its original coordinate (so we can
  // add an undoer before).
  if (m_celStartX != m_celNewX ||
      m_celStartY != m_celNewY) {
    // Put the cel in the original position.
    if (m_cel)
      m_cel->setPosition(m_celStartX, m_celStartY);

    // If the user didn't cancel the operation...
    if (!m_canceled) {
      ContextWriter writer(UIContext::instance());
      UndoTransaction undoTransaction(writer.context(), "Cel Movement", undo::ModifyDocument);
      DocumentApi api = document->getApi();

      // And now we move the cel to the new position.
      if (m_cel)
        api.setCelPosition(writer.sprite(), m_cel, m_celNewX, m_celNewY);

      // Move selection if it was visible
      if (m_maskVisible)
        api.setMaskPosition(document->getMask()->getBounds().x + m_celNewX - m_celStartX,
                            document->getMask()->getBounds().y + m_celNewY - m_celStartY);

      undoTransaction.commit();
    }
  }

  // Restore the mask visibility.
  if (m_maskVisible) {
    document->setMaskVisible(m_maskVisible);
    document->generateMaskBoundaries();
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingCelState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  int newMouseX, newMouseY;
  editor->screenToEditor(msg->position().x, msg->position().y, &newMouseX, &newMouseY);

  m_celNewX = m_celStartX - m_mouseStartX + newMouseX;
  m_celNewY = m_celStartY - m_mouseStartY + newMouseY;

  if (m_cel)
    m_cel->setPosition(m_celNewX, m_celNewY);

  // Redraw the new cel position.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingCelState::onUpdateStatusBar(Editor* editor)
{
  StatusBar::instance()->setStatusText
    (0,
     "Pos %3d %3d Offset %3d %3d",
     (int)m_celNewX,
     (int)m_celNewY,
     (int)(m_celNewX - m_celStartX),
     (int)(m_celNewY - m_celStartY));

  return true;
}

} // namespace app

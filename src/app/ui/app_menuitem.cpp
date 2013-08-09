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

#include "app/ui/app_menuitem.h"

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/modules/gui.h"
#include "app/ui_context.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/widget.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace app {

using namespace ui;

AppMenuItem::AppMenuItem(const char* text, Command* command, Params* params)
 : MenuItem(text)
 , m_command(command)
 , m_params(params ? params->clone(): NULL)
{
}

AppMenuItem::~AppMenuItem()
{
  delete m_params;
}

bool AppMenuItem::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      UIContext* context = UIContext::instance();

      if (m_command) {
        if (m_params)
          m_command->loadParams(m_params);

        setEnabled(m_command->isEnabled(context));
        setSelected(m_command->isChecked(context));
      }
      break;
    }

    case kCloseMessage:
      // disable the menu (the keyboard shortcuts are processed by "manager_msg_proc")
      setEnabled(false);
      break;

    default:
      if (msg->type() == kOpenMenuItemMessage) {
        // Update the context flags after opening the menuitem's
        // submenu to update the "enabled" flag of each command
        // correctly.
        Context* context = UIContext::instance();
        context->updateFlags();
      }
      break;
  }

  return MenuItem::onProcessMessage(msg);
}

void AppMenuItem::onClick()
{
  MenuItem::onClick();

  if (m_command) {
    if (m_params)
      m_command->loadParams(m_params);

    UIContext* context = UIContext::instance();
    if (m_command->isEnabled(context))
      context->executeCommand(m_command);
  }
}

} // namespace app

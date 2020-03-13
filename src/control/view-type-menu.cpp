/*
 * Peony-Qt
 *
 * Copyright (C) 2020, Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "view-type-menu.h"

#include "view-factory-sort-filter-model.h"
#include "directory-view-factory-manager.h"

#include <QActionGroup>

#include <QDebug>

ViewTypeMenu::ViewTypeMenu(QWidget *parent) : QMenu(parent)
{
    m_model = new Peony::ViewFactorySortFilterModel2(this);
    m_view_actions = new QActionGroup(this);
    m_view_actions->setExclusive(true);

    connect(m_view_actions, &QActionGroup::triggered, this, [=](QAction *action){
        auto viewId = action->text();
        setCurrentView(viewId);
    });

    connect(this, &QMenu::aboutToShow, this, [=](){
        qDebug()<<"show menu";
        updateMenuActions();
    });

    setCurrentDirectory("file:///");
    auto viewFactory = Peony::DirectoryViewFactoryManager2::getInstance();
    setCurrentView(viewFactory->getDefaultViewId());
}

void ViewTypeMenu::setCurrentView(const QString &viewId)
{
    if (viewId == m_current_view_id)
        return;

    if (isViewIdValid(viewId)) {
        m_current_view_id = viewId;
    }

    for (auto action : m_view_actions->actions()) {
        if (action->text() == viewId) {
            action->setChecked(true);
        }
    }

    Q_EMIT this->switchViewRequest(viewId, m_model->iconFromViewId(viewId));
}

void ViewTypeMenu::setCurrentDirectory(const QString &uri)
{
    m_current_uri = uri;
    m_model->setDirectoryUri(uri);
}

bool ViewTypeMenu::isViewIdValid(const QString &viewId)
{
    return m_model->supportViewIds().contains(viewId);
}

void ViewTypeMenu::updateMenuActions()
{
    auto supportViews = m_model->supportViewIds();
    for (auto action : m_view_actions->actions()) {
        removeAction(action);
        m_view_actions->removeAction(action);
        action->deleteLater();
    }
    for (auto id : supportViews) {
        auto action = new QAction(this);
        action->setText(id);
        action->setIcon(m_model->iconFromViewId(id));
        m_view_actions->addAction(action);
        addAction(action);

        action->setCheckable(true);
        if (m_current_view_id == id) {
            action->setChecked(true);
        }
    }
}
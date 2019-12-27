/*
 * Peony-Qt's Library
 *
 * Copyright (C) 2019, Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: Yue Lan <lanyue@kylinos.cn>
 *
 */

#include "volume-manager.h"
#include "file-utils.h"
#include "gobject-template.h"
#include <QMessageBox>

#include <QDebug>

using namespace Peony;

static VolumeManager *m_global_manager = nullptr;

VolumeManager::VolumeManager(QObject *parent) : QObject(parent)
{
    m_volume_monitor = g_volume_monitor_get();

    m_drive_connected_handle = g_signal_connect(m_volume_monitor,
                                                "drive-connected",
                                                G_CALLBACK(drive_connected_callback),
                                                this);

    m_drive_disconnected_handle = g_signal_connect(m_volume_monitor,
                                                   "drive-disconnected",
                                                   G_CALLBACK(drive_disconnected_callback),
                                                   this);

    m_volume_added_handle = g_signal_connect(m_volume_monitor,
                                             "volume-added",
                                             G_CALLBACK(volume_added_callback),
                                             this);

    m_volume_removed_handle = g_signal_connect(m_volume_monitor,
                                               "volume-removed",
                                               G_CALLBACK(volume_removed_callback),
                                               this);

    m_mount_added_handle = g_signal_connect(m_volume_monitor,
                                            "mount-added",
                                            G_CALLBACK(mount_added_callback),
                                            this);

    m_mount_removed_handle = g_signal_connect(m_volume_monitor,
                                              "mount-removed",
                                              G_CALLBACK(mount_removed_callback),
                                              this);
}

VolumeManager::~VolumeManager()
{
    disconnect();

    g_signal_handler_disconnect(m_volume_monitor, m_drive_connected_handle);
    g_signal_handler_disconnect(m_volume_monitor, m_drive_disconnected_handle);
    g_signal_handler_disconnect(m_volume_monitor, m_volume_added_handle);
    g_signal_handler_disconnect(m_volume_monitor, m_volume_removed_handle);
    g_signal_handler_disconnect(m_volume_monitor, m_mount_added_handle);
    g_signal_handler_disconnect(m_volume_monitor, m_mount_removed_handle);

    g_object_unref(m_volume_monitor);
}

VolumeManager *VolumeManager::getInstance()
{
    if (!m_global_manager) {
        m_global_manager = new VolumeManager;
    }
    return m_global_manager;
}

void VolumeManager::drive_connected_callback(GVolumeMonitor *monitor,
                                             GDrive *drive,
                                             VolumeManager *p_this)
{
    Q_UNUSED(monitor);
    auto data = std::make_shared<Drive>(drive);
    Q_EMIT p_this->driveConnected(data);
}

void VolumeManager::drive_disconnected_callback(GVolumeMonitor *monitor,
                                                GDrive *drive,
                                                VolumeManager *p_this)
{
    Q_UNUSED(monitor);
    Q_EMIT p_this->driveDisconnected(std::make_shared<Drive>(drive));
}

void VolumeManager::volume_added_callback(GVolumeMonitor *monitor,
                                          GVolume *volume,
                                          VolumeManager *p_this)
{
    Q_UNUSED(monitor);
    Q_EMIT p_this->volumeAdded(std::make_shared<Volume>(volume));
}

void VolumeManager::volume_removed_callback(GVolumeMonitor *monitor,
                                            GVolume *volume,
                                            VolumeManager *p_this)
{
    Q_UNUSED(monitor);
    Q_EMIT p_this->volumeRemoved(std::make_shared<Volume>(volume));
}

void VolumeManager::mount_added_callback(GVolumeMonitor *monitor,
                                         GMount *mount,
                                         VolumeManager *p_this)
{
    Q_UNUSED(monitor);
    Q_EMIT p_this->mountAdded(std::make_shared<Mount>(mount));
}

void VolumeManager::mount_removed_callback(GVolumeMonitor *monitor,
                                           GMount *mount,
                                           VolumeManager *p_this)
{
    Q_UNUSED(monitor);
    Q_EMIT p_this->mountRemoved(std::make_shared<Mount>(mount));
}

void VolumeManager::unmount_cb(GFile *file, GAsyncResult *result, GError **error)
{
    bool successed = g_file_unmount_mountable_with_operation_finish(file, result, error);
    if (!successed) {
        if (error) {
            auto err = GErrorWrapper::wrapFrom(g_error_copy(*error));
            QMessageBox::warning(nullptr, tr("Error"), err.get()->message());
        }
    }
}

void VolumeManager::unmount(const QString &uri)
{
    auto file = wrapGFile(g_file_new_for_uri(uri.toUtf8().constData()));
    g_file_unmount_mountable_with_operation(file.get()->get(),
                                            G_MOUNT_UNMOUNT_NONE,
                                            nullptr,
                                            nullptr,
                                            GAsyncReadyCallback(unmount_cb),
                                            nullptr);
}

//FIXME: should i add async support?
std::shared_ptr<Mount> VolumeManager::getMountFromUri(const QString &uri)
{
    GFile *file = g_file_new_for_uri(uri.toUtf8().constData());
    if (!file)
        return nullptr;

    std::shared_ptr<Mount> tmp;
    GError *err = nullptr;
    GMount *mount = g_file_find_enclosing_mount(file, nullptr, &err);
    if (err) {
        qDebug()<<err->message;
        g_error_free(err);
    }
    g_object_unref(file);
    if (mount) {
        tmp = std::make_shared<Mount>(mount, true);
    }
    return tmp;
}

std::shared_ptr<Drive> VolumeManager::getDriveFromUri(const QString &uri)
{
    GFile *file = g_file_new_for_uri(uri.toUtf8().constData());
    if (!file)
        return nullptr;

    std::shared_ptr<Drive> tmp;
    GMount *mount = g_file_find_enclosing_mount(file, nullptr, nullptr);
    g_object_unref(file);
    if (mount) {
        GDrive *drive = g_mount_get_drive(mount);
        if (drive) {
            tmp = std::make_shared<Drive>(drive, true);
        }

        g_object_unref(mount);
    }
    return tmp;
}

std::shared_ptr<Volume> VolumeManager::getVolumeFromUri(const QString &uri)
{
    GFile *file = g_file_new_for_uri(uri.toUtf8().constData());
    if (!file)
        return nullptr;

    std::shared_ptr<Volume> tmp;
    GMount *mount = g_file_find_enclosing_mount(file, nullptr, nullptr);
    g_object_unref(file);
    if (mount) {
        GVolume *volume = g_mount_get_volume(mount);
        if (volume) {
            tmp = std::make_shared<Volume>(volume, true);
        }

        g_object_unref(mount);
    }
    return tmp;
}

std::shared_ptr<Drive> VolumeManager::getDriveFromMount(const std::shared_ptr<Mount> &mount)
{
    GMount *m = mount->getGMount();
    if (!m) {
        return nullptr;
    }
    GDrive *drive = g_mount_get_drive(m);
    std::shared_ptr<Drive> tmp;
    if (!drive) {
        return nullptr;
    }
    tmp = std::make_shared<Drive>(drive, true);
    return tmp;
}

std::shared_ptr<Volume> VolumeManager::getVolumeFromMount(const std::shared_ptr<Mount> &mount)
{
    GMount *m = mount->getGMount();
    if (!m) {
        return nullptr;
    }
    GVolume *volume = g_mount_get_volume(m);
    std::shared_ptr<Volume> tmp;
    if (!volume) {
        return nullptr;
    }
    tmp = std::make_shared<Volume>(volume, true);
    return tmp;
}

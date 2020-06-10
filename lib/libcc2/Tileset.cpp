/******************************************************************************
 * This file is part of CCTools.                                              *
 *                                                                            *
 * CCTools is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * CCTools is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with CCTools.  If not, see <http://www.gnu.org/licenses/>.           *
 ******************************************************************************/

#include "Tileset.h"

#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include "libcc1/Stream.h"

static quint8 read8(QFile& file)
{
    quint8 value;
    file.read((char*)&value, sizeof(quint8));
    return value;
}

static quint32 read32(QFile& file)
{
    quint32 value;
    file.read((char*)&value, sizeof(quint32));
    return SWAP32(value);
}

void CC2ETileset::load(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
        throw ccl::IOException("Cannot open tileset file for reading");

    char magic[8];
    if (file.read(magic, 8) != 8 || memcmp(magic, "CCTILE02", 8) != 0) {
        file.close();
        throw ccl::IOException("Invalid Tileset format");
    }

    quint32 len;
    std::unique_ptr<char[]> utfbuffer;
    std::unique_ptr<uchar[]> pixbuffer;
    QPixmap tempmap;

    // Tileset name
    len = read32(file);
    utfbuffer.reset(new char[len]);
    file.read(utfbuffer.get(), len);
    m_name = QString::fromUtf8(utfbuffer.get(), len);

    // Description
    len = read32(file);
    utfbuffer.reset(new char[len]);
    file.read(utfbuffer.get(), len);
    m_description = QString::fromUtf8(utfbuffer.get(), len);

    // Tile size
    m_size = (int)read8(file);

    // Skip CC1 tilesets
    len = read32(file);
    pixbuffer.reset(new uchar[len]);
    file.read((char*)pixbuffer.get(), len);
    len = read32(file);
    pixbuffer.reset(new uchar[len]);
    file.read((char*)pixbuffer.get(), len);

    // CC2 tiles
    len = read32(file);
    pixbuffer.reset(new uchar[len]);
    file.read((char*)pixbuffer.get(), len);
    if (!tempmap.loadFromData(pixbuffer.get(), len, "PNG")) {
        file.close();
        throw ccl::IOException("Invalid or corrupt CC2 image");
    }
    for (int i = 0; i < cc2::NUM_GRAPHICS; ++i)
        m_gfx[i] = tempmap.copy((i / 16) * m_size, (i % 16) * m_size, m_size, m_size);

    file.close();
    m_filename = QFileInfo(filename).fileName();
}

void CC2ETileset::drawAt(QPainter& painter, int x, int y, const cc2::Tile* tile) const
{
    // Recurse up from the bottom-most layer
    if (tile->haveLower())
        drawAt(painter, x, y, tile->lower());

    // Draw the base tile
    switch (tile->type()) {
    case cc2::Tile::Floor:
        if (tile->modifier() != 0) {
            // TODO: Draw wires properly
            painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_Floor_Wire4]);
        } else {
            painter.drawPixmap(x, y, m_gfx[cc2::G_Floor]);
        }
        break;
    case cc2::Tile::Wall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Wall]);
        break;
    case cc2::Tile::Ice:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Ice]);
        break;
    case cc2::Tile::Ice_NE:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Ice_NE]);
        break;
    case cc2::Tile::Ice_SE:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Ice_SE]);
        break;
    case cc2::Tile::Ice_SW:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Ice_SW]);
        break;
    case cc2::Tile::Ice_NW:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Ice_NW]);
        break;
    case cc2::Tile::Water:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Water]);
        break;
    case cc2::Tile::Fire:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Fire]);
        break;
    case cc2::Tile::Force_N:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Force_N]);
        break;
    case cc2::Tile::Force_E:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Force_E]);
        break;
    case cc2::Tile::Force_S:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Force_S]);
        break;
    case cc2::Tile::Force_W:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Force_W]);
        break;
    case cc2::Tile::ToggleWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_ToggleWall]);
        break;
    case cc2::Tile::ToggleFloor:
        painter.drawPixmap(x, y, m_gfx[cc2::G_ToggleFloor]);
        break;
    case cc2::Tile::Teleport_Red:
        // TODO: Draw wires properly
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_Teleport_Red]);
        break;
    case cc2::Tile::Teleport_Blue:
        // TODO: Draw wires properly
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_Teleport_Blue]);
        break;
    case cc2::Tile::Teleport_Yellow:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Teleport_Yellow]);
        break;
    case cc2::Tile::Teleport_Green:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Teleport_Green]);
        break;
    case cc2::Tile::Exit:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Exit]);
        break;
    case cc2::Tile::Slime:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Slime]);
        break;
    case cc2::Tile::MirrorPlayer:
        painter.drawPixmap(x, y, m_gfx[cc2::G_MirrorPlayer_Underlay]);
        /* fall through */
    case cc2::Tile::Player:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player_S]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::DirtBlock:
        if (tile->haveLower() && tile->lower()->needXray())
            painter.drawPixmap(x, y, m_gfx[cc2::G_DirtBlock_Xray]);
        else
            painter.drawPixmap(x, y, m_gfx[cc2::G_DirtBlock]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::Walker:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Walker]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::Ship:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ship_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ship_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ship_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ship_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ship_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::IceBlock:
        if (tile->haveLower() && tile->lower()->needXray())
            painter.drawPixmap(x, y, m_gfx[cc2::G_IceBlock_Xray]);
        else
            painter.drawPixmap(x, y, m_gfx[cc2::G_IceBlock]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::UNUSED_Barrier_S:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_S]);
        break;
    case cc2::Tile::UNUSED_Barrier_E:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_E]);
        break;
    case cc2::Tile::UNUSED_Barrier_SE:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_S]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_E]);
        break;
    case cc2::Tile::Gravel:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Gravel]);
        break;
    case cc2::Tile::ToggleButton:
        painter.drawPixmap(x, y, m_gfx[cc2::G_ToggleButton]);
        break;
    case cc2::Tile::TankButton:
        painter.drawPixmap(x, y, m_gfx[cc2::G_TankButton]);
        break;
    case cc2::Tile::BlueTank:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueTank_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueTank_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueTank_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueTank_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueTank_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::Door_Red:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Door_Red]);
        break;
    case cc2::Tile::Door_Blue:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Door_Blue]);
        break;
    case cc2::Tile::Door_Yellow:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Door_Yellow]);
        break;
    case cc2::Tile::Door_Green:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Door_Green]);
        break;
    case cc2::Tile::Key_Red:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Key_Red]);
        break;
    case cc2::Tile::Key_Blue:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Key_Blue]);
        break;
    case cc2::Tile::Key_Yellow:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Key_Yellow]);
        break;
    case cc2::Tile::Key_Green:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Key_Green]);
        break;
    case cc2::Tile::Chip:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Chip]);
        break;
    case cc2::Tile::ExtraChip:
        painter.drawPixmap(x, y, m_gfx[cc2::G_ExtraChip]);
        break;
    case cc2::Tile::Socket:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Socket]);
        break;
    case cc2::Tile::PopUpWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_PopUpWall]);
        break;
    case cc2::Tile::AppearingWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_AppearingWall]);
        break;
    case cc2::Tile::InvisWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_InvisWall]);
        break;
    case cc2::Tile::BlueWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_BlueWall]);
        break;
    case cc2::Tile::BlueFloor:
        painter.drawPixmap(x, y, m_gfx[cc2::G_BlueFloor]);
        break;
    case cc2::Tile::Dirt:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Dirt]);
        break;
    case cc2::Tile::Ant:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ant_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ant_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ant_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ant_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ant_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::Centipede:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Centipede_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Centipede_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Centipede_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Centipede_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Centipede_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::Ball:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Ball]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::Blob:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Blob]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::AngryTeeth:
        switch (tile->direction()) {
        case cc2::Tile::North:
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AngryTeeth_S]);
            drawArrow(painter, x, y, tile->direction());
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AngryTeeth_E]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AngryTeeth_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AngryTeeth_S]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::FireBox:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FireBox]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::CloneButton:
        painter.drawPixmap(x, y, m_gfx[cc2::G_CloneButton]);
        break;
    case cc2::Tile::TrapButton:
        painter.drawPixmap(x, y, m_gfx[cc2::G_TrapButton]);
        break;
    case cc2::Tile::IceCleats:
        painter.drawPixmap(x, y, m_gfx[cc2::G_IceCleats]);
        break;
    case cc2::Tile::MagnoShoes:
        painter.drawPixmap(x, y, m_gfx[cc2::G_MagnoShoes]);
        break;
    case cc2::Tile::FireShoes:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FireShoes]);
        break;
    case cc2::Tile::Flippers:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Flippers]);
        break;
    case cc2::Tile::ToolThief:
        painter.drawPixmap(x, y, m_gfx[cc2::G_ToolThief]);
        break;
    case cc2::Tile::RedBomb:
        painter.drawPixmap(x, y, m_gfx[cc2::G_RedBomb]);
        break;
    //case cc2::Tile::UNUSED_41:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_Trap]);
    //    break;
    case cc2::Tile::Trap:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Trap]);
        break;
    case cc2::Tile::UNUSED_Cloner:
        // TODO: Need a better graphic?
        painter.drawPixmap(x, y, m_gfx[cc2::G_Cloner]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
        break;
    case cc2::Tile::Cloner:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Cloner]);
        // TODO: Render modifier arrows
        break;
    case cc2::Tile::Clue:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Clue]);
        break;
    case cc2::Tile::Force_Rand:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Force_Rand]);
        break;
    case cc2::Tile::AreaCtlButton:
        painter.drawPixmap(x, y, m_gfx[cc2::G_AreaCtlButton]);
        break;
    case cc2::Tile::RevolvDoor_SW:
        painter.drawPixmap(x, y, m_gfx[cc2::G_RevolvDoor_SW]);
        break;
    case cc2::Tile::RevolvDoor_NW:
        painter.drawPixmap(x, y, m_gfx[cc2::G_RevolvDoor_NW]);
        break;
    case cc2::Tile::RevolvDoor_NE:
        painter.drawPixmap(x, y, m_gfx[cc2::G_RevolvDoor_NE]);
        break;
    case cc2::Tile::RevolvDoor_SE:
        painter.drawPixmap(x, y, m_gfx[cc2::G_RevolvDoor_SE]);
        break;
    case cc2::Tile::TimeBonus:
        painter.drawPixmap(x, y, m_gfx[cc2::G_TimeBonus]);
        break;
    case cc2::Tile::ToggleClock:
        painter.drawPixmap(x, y, m_gfx[cc2::G_ToggleClock]);
        break;
    case cc2::Tile::Transformer:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Transformer]);
        break;
    case cc2::Tile::TrainTracks:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Gravel]);
        // TODO: Render tracks and rails
        break;
    case cc2::Tile::SteelWall:
        if (tile->modifier() != 0) {
            // TODO: Draw wires properly
            painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_SteelWall_Wire4]);
        } else {
            painter.drawPixmap(x, y, m_gfx[cc2::G_SteelWall]);
        }
        break;
    case cc2::Tile::TimeBomb:
        painter.drawPixmap(x, y, m_gfx[cc2::G_TimeBomb]);
        break;
    case cc2::Tile::Helmet:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Helmet]);
        break;
    //case cc2::Tile::UNUSED_53:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_PopDownGWall]);
    //    drawArrow(painter, x, y, tile->direction());
    //    break;
    //case cc2::Tile::UNUSED_54:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_ToggleFloor]);
    //    break;
    //case cc2::Tile::UNUSED_55:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_ToggleFloor]);
    //    break;
    case cc2::Tile::MirrorPlayer2:
        painter.drawPixmap(x, y, m_gfx[cc2::G_MirrorPlayer_Underlay]);
        /* fall through */
    case cc2::Tile::Player2:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player2_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player2_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player2_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player2_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Player2_S]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::TimidTeeth:
        switch (tile->direction()) {
        case cc2::Tile::North:
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_TimidTeeth_S]);
            drawArrow(painter, x, y, tile->direction());
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_TimidTeeth_E]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_TimidTeeth_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_TimidTeeth_S]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    //case cc2::Tile::UNUSED_Explosion:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_??]);
    //    break;
    case cc2::Tile::HikingBoots:
        painter.drawPixmap(x, y, m_gfx[cc2::G_HikingBoots]);
        break;
    case cc2::Tile::MaleOnly:
        painter.drawPixmap(x, y, m_gfx[cc2::G_MaleOnly]);
        break;
    case cc2::Tile::FemaleOnly:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FemaleOnly]);
        break;
    case cc2::Tile::LogicGate:
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        switch (tile->modifier()) {
        case cc2::TileModifier::Inverter_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Inverter_N]);
            break;
        case cc2::TileModifier::Inverter_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Inverter_E]);
            break;
        case cc2::TileModifier::Inverter_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Inverter_S]);
            break;
        case cc2::TileModifier::Inverter_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Inverter_W]);
            break;
        case cc2::TileModifier::AndGate_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AndGate_N]);
            break;
        case cc2::TileModifier::AndGate_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AndGate_E]);
            break;
        case cc2::TileModifier::AndGate_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AndGate_S]);
            break;
        case cc2::TileModifier::AndGate_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_AndGate_W]);
            break;
        case cc2::TileModifier::OrGate_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_OrGate_N]);
            break;
        case cc2::TileModifier::OrGate_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_OrGate_E]);
            break;
        case cc2::TileModifier::OrGate_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_OrGate_S]);
            break;
        case cc2::TileModifier::OrGate_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_OrGate_W]);
            break;
        case cc2::TileModifier::XorGate_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_XorGate_N]);
            break;
        case cc2::TileModifier::XorGate_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_XorGate_E]);
            break;
        case cc2::TileModifier::XorGate_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_XorGate_S]);
            break;
        case cc2::TileModifier::XorGate_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_XorGate_W]);
            break;
        case cc2::TileModifier::LatchGateCW_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCW_N]);
            break;
        case cc2::TileModifier::LatchGateCW_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCW_E]);
            break;
        case cc2::TileModifier::LatchGateCW_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCW_S]);
            break;
        case cc2::TileModifier::LatchGateCW_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCW_W]);
            break;
        case cc2::TileModifier::NandGate_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_NandGate_N]);
            break;
        case cc2::TileModifier::NandGate_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_NandGate_E]);
            break;
        case cc2::TileModifier::NandGate_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_NandGate_S]);
            break;
        case cc2::TileModifier::NandGate_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_NandGate_W]);
            break;
        case cc2::TileModifier::CounterGate_0:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_0]);
            break;
        case cc2::TileModifier::CounterGate_1:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_1]);
            break;
        case cc2::TileModifier::CounterGate_2:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_2]);
            break;
        case cc2::TileModifier::CounterGate_3:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_3]);
            break;
        case cc2::TileModifier::CounterGate_4:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_4]);
            break;
        case cc2::TileModifier::CounterGate_5:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_5]);
            break;
        case cc2::TileModifier::CounterGate_6:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_6]);
            break;
        case cc2::TileModifier::CounterGate_7:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_7]);
            break;
        case cc2::TileModifier::CounterGate_8:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_8]);
            break;
        case cc2::TileModifier::CounterGate_9:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CounterGate_9]);
            break;
        case cc2::TileModifier::LatchGateCCW_N:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCCW_N]);
            break;
        case cc2::TileModifier::LatchGateCCW_E:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCCW_E]);
            break;
        case cc2::TileModifier::LatchGateCCW_S:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCCW_S]);
            break;
        case cc2::TileModifier::LatchGateCCW_W:
            painter.drawPixmap(x, y, m_gfx[cc2::G_LatchGateCCW_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Inverter_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    //case cc2::Tile::UNUSED_5d:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_Player]);
    //    break;
    case cc2::Tile::LogicSwitch:
        // TODO: Draw wires properly
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_LogicSwitch]);
        break;
    case cc2::Tile::FlameJet_Off:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FlameJet_Off]);
        break;
    case cc2::Tile::FlameJet_On:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FlameJet_On]);
        break;
    case cc2::Tile::FlameJetButton:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FlameJetButton]);
        break;
    case cc2::Tile::Lightning:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Lightning]);
        break;
    case cc2::Tile::YellowTank:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowTank_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowTank_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowTank_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowTank_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowTank_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::YellowTankCtrl:
        painter.drawPixmap(x, y, m_gfx[cc2::G_YellowTankCtrl]);
        break;
    //case cc2::Tile::UNUSED_67:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_BowlingBall]);
    //    break;
    case cc2::Tile::BowlingBall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_BowlingBall]);
        break;
    case cc2::Tile::Rover:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Rover_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Rover_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Rover_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Rover_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Rover_N]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::TimePenalty:
        painter.drawPixmap(x, y, m_gfx[cc2::G_TimePenalty]);
        break;
    case cc2::Tile::StyledFloor:
        switch (tile->modifier()) {
        case cc2::TileModifier::CamoTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CamoCFloor]);
            break;
        case cc2::TileModifier::PinkDotsTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_PinkDotsCFloor]);
            break;
        case cc2::TileModifier::YellowBrickTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowBrickCFloor]);
            break;
        case cc2::TileModifier::BlueTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueCFloor]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CamoCFloor]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    //case cc2::Tile::UNUSED_6c:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_??]);
    //    break;
    case cc2::Tile::PanelCanopy:
        if (tile->panelFlags() & cc2::Tile::Canopy) {
            if (tile->haveLower() && tile->lower()->needXray())
                painter.drawPixmap(x, y, m_gfx[cc2::G_Canopy_Xray]);
            else
                painter.drawPixmap(x, y, m_gfx[cc2::G_Canopy]);
        }
        if (tile->panelFlags() & cc2::Tile::PanelNorth)
            painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_N]);
        if (tile->panelFlags() & cc2::Tile::PanelEast)
            painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_E]);
        if (tile->panelFlags() & cc2::Tile::PanelSouth)
            painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_S]);
        if (tile->panelFlags() & cc2::Tile::PanelWest)
            painter.drawPixmap(x, y, m_gfx[cc2::G_Panel_W]);
        if (tile->panelFlags() == 0) {
            painter.drawPixmap(x, y, m_gfx[cc2::G_Canopy_Xray]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
        }
        break;
    //case cc2::Tile::UNUSED_6e:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_RRSign]);
    //    break;
    case cc2::Tile::RRSign:
        painter.drawPixmap(x, y, m_gfx[cc2::G_RRSign]);
        break;
    case cc2::Tile::StyledWall:
        switch (tile->modifier()) {
        case cc2::TileModifier::CamoTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CamoCWall]);
            break;
        case cc2::TileModifier::PinkDotsTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_PinkDotsCWall]);
            break;
        case cc2::TileModifier::YellowBrickTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_YellowBrickCWall]);
            break;
        case cc2::TileModifier::BlueTheme:
            painter.drawPixmap(x, y, m_gfx[cc2::G_BlueCWall]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_CamoCWall]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::AsciiGlyph:
        painter.drawPixmap(x, y, m_gfx[cc2::G_AsciiGlyphFrame]);
        drawGlyph(painter, x, y, tile->modifier());
        break;
    case cc2::Tile::LSwitchFloor:
        painter.drawPixmap(x, y, m_gfx[cc2::G_LSwitchFloor]);
        break;
    case cc2::Tile::LSwitchWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_LSwitchWall]);
        break;
    //case cc2::Tile::UNUSED_74:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_??]);
    //    break;
    //case cc2::Tile::UNUSED_75:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_??]);
    //    break;
    //case cc2::Tile::UNUSED_79:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_??]);
    //    drawArrow(painter, x, y, tile->direction());
    //    break;
    case cc2::Tile::Flag10:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Flag10]);
        break;
    case cc2::Tile::Flag100:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Flag100]);
        break;
    case cc2::Tile::Flag1000:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Flag1000]);
        break;
    case cc2::Tile::StayUpGWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_StayUpGWall]);
        break;
    case cc2::Tile::PopDownGWall:
        painter.drawPixmap(x, y, m_gfx[cc2::G_PopDownGWall]);
        break;
    case cc2::Tile::Disallow:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Disallow]);
        break;
    case cc2::Tile::Flag2x:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Flag2x]);
        break;
    case cc2::Tile::DirBlock:
        painter.drawPixmap(x, y, m_gfx[cc2::G_DirBlock]);
        // TODO: Draw arrows
        break;
    case cc2::Tile::FloorMimic:
        painter.drawPixmap(x, y, m_gfx[cc2::G_FloorMimic]);
        drawArrow(painter, x, y, tile->direction());
        break;
    case cc2::Tile::GreenBomb:
        painter.drawPixmap(x, y, m_gfx[cc2::G_GreenBomb]);
        break;
    case cc2::Tile::GreenChip:
        painter.drawPixmap(x, y, m_gfx[cc2::G_GreenChip]);
        break;
    //case cc2::Tile::UNUSED_85:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_GreenBomb]);
    //    break;
    //case cc2::Tile::UNUSED_86:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_GreenChip]);
    //    break;
    case cc2::Tile::RevLogicButton:
        // TODO: Draw wires properly
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_RevLogicButton]);
        break;
    case cc2::Tile::Switch_Off:
        // TODO: Draw wires properly
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_Switch_Off]);
        break;
    case cc2::Tile::Switch_On:
        // TODO: Draw wires properly
        painter.drawPixmap(x, y, m_gfx[cc2::G_WireFill]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_Switch_On]);
        break;
    case cc2::Tile::KeyThief:
        painter.drawPixmap(x, y, m_gfx[cc2::G_KeyThief]);
        break;
    case cc2::Tile::Ghost:
        switch (tile->direction()) {
        case cc2::Tile::North:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ghost_N]);
            break;
        case cc2::Tile::East:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ghost_E]);
            break;
        case cc2::Tile::South:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ghost_S]);
            break;
        case cc2::Tile::West:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ghost_W]);
            break;
        default:
            painter.drawPixmap(x, y, m_gfx[cc2::G_Ghost_S]);
            painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
            break;
        }
        break;
    case cc2::Tile::SteelFoil:
        painter.drawPixmap(x, y, m_gfx[cc2::G_SteelFoil]);
        break;
    case cc2::Tile::Turtle:
        // TODO: Maybe we don't need a masked version of the turtle...
        painter.drawPixmap(x, y, m_gfx[cc2::G_Water]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_Turtle]);
        break;
    case cc2::Tile::Eye:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Eye]);
        break;
    case cc2::Tile::Bribe:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Bribe]);
        break;
    case cc2::Tile::SpeedShoes:
        painter.drawPixmap(x, y, m_gfx[cc2::G_SpeedShoes]);
        break;
    //case cc2::Tile::UNUSED_91:
    //    painter.drawPixmap(x, y, m_gfx[cc2::G_Canopy]);
    //    break;
    case cc2::Tile::Hook:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Hook]);
        break;
    default:
        painter.drawPixmap(x, y, m_gfx[cc2::G_Floor]);
        painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
        if (tile->haveDirection())
            drawArrow(painter, x, y, tile->direction());
        break;
    }
}

void CC2ETileset::drawArrow(QPainter& painter, int x, int y,
                            cc2::Tile::Direction direction) const
{
    switch (direction) {
    case cc2::Tile::North:
        painter.drawPixmap(x + (m_size / 4), y, m_gfx[cc2::G_GlyphArrows],
                           0, 0, m_size / 2, m_size / 2);
        break;
    case cc2::Tile::East:
        painter.drawPixmap(x + (m_size / 2), y + (m_size / 4), m_gfx[cc2::G_GlyphArrows],
                           m_size / 2, 0, m_size / 2, m_size / 2);
        break;
    case cc2::Tile::South:
        painter.drawPixmap(x + (m_size / 4), y + (m_size / 2), m_gfx[cc2::G_GlyphArrows],
                           0, m_size / 2, m_size / 2, m_size / 2);
        break;
    case cc2::Tile::West:
        painter.drawPixmap(x, y + (m_size / 4), m_gfx[cc2::G_GlyphArrows],
                           m_size / 2, m_size / 2, m_size / 2, m_size / 2);
        break;
    default:
        break;
    }
}

void CC2ETileset::drawGlyph(QPainter& painter, int x, int y, uint32_t glyph) const
{
    if (glyph < cc2::TileModifier::GlyphMIN || glyph > cc2::TileModifier::GlyphMAX) {
        painter.drawPixmap(x, y, m_gfx[cc2::G_InvalidBase]);
        return;
    }

    const size_t id = cc2::G_GlyphArrows + ((glyph - cc2::TileModifier::GlyphMIN) / 4);
    const int sx = (glyph % 2) * (m_size / 2);
    const int sy = ((glyph / 2) % 2) * (m_size / 2);
    painter.drawPixmap(x + (m_size / 4), y + (m_size / 4), m_gfx[id],
                       sx, sy, m_size / 2, m_size / 2);
}
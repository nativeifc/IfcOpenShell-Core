/********************************************************************************
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

// uncomment if you'd like negative or close to zero extrusion depths to succeed
// #define PERMISSIVE_EXTRUSION

#include "mapping.h"
#define mapping POSTFIX_SCHEMA(mapping)
using namespace ifcopenshell::geometry;

taxonomy::ptr mapping::map_impl(const IfcSchema::IfcExtrudedAreaSolid* inst) {
	const double height = inst->Depth() * length_unit_;
	if (height < conv_settings_.getValue(ConversionSettings::GV_PRECISION)) {
		Logger::Message(Logger::LOG_ERROR, "Non-positive extrusion height encountered for:", inst);
#ifndef PERMISSIVE_EXTRUSION
		return nullptr;
#endif
	}

	taxonomy::matrix4::ptr matrix;
	bool has_position = true;
#ifdef SCHEMA_IfcSweptAreaSolid_Position_IS_OPTIONAL
	has_position = inst->Position() != nullptr;
#endif
	if (has_position) {
		matrix = taxonomy::cast<taxonomy::matrix4>(map(inst->Position()));
	}

#ifdef PERMISSIVE_EXTRUSION
	if (abs(height) < getValue(GV_PRECISION)) {
		face->matrix = matrix;
		return face;
	}
#endif

	return taxonomy::make<taxonomy::extrusion>(
		matrix,
		taxonomy::cast<taxonomy::face>(map(inst->SweptArea())),
		taxonomy::cast<taxonomy::direction3>(map(inst->ExtrudedDirection())),
		height
	);
}

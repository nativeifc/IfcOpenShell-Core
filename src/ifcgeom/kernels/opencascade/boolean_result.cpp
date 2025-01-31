#include "OpenCascadeKernel.h"

#include "boolean_utils.h"
#include "base_utils.h"

using namespace IfcGeom;
using namespace ifcopenshell::geometry;
using namespace ifcopenshell::geometry::kernels;

namespace {
	BOPAlgo_Operation op_to_occt(taxonomy::boolean_result::operation_t t) {
		switch (t) {
		case taxonomy::boolean_result::UNION: return BOPAlgo_FUSE;
		case taxonomy::boolean_result::INTERSECTION: return BOPAlgo_COMMON;
		case taxonomy::boolean_result::SUBTRACTION: return BOPAlgo_CUT;
		}
	}

	bool get_single_child(const TopoDS_Shape& s, TopoDS_Shape& child) {
		TopoDS_Iterator it(s);
		if (!it.More()) {
			return false;
		}
		child = it.Value();
		it.Next();
		return !it.More();
	}

	bool is_unbounded_halfspace(const TopoDS_Shape& solid) {
		if (solid.ShapeType() != TopAbs_SOLID) {
			return false;
		}
		TopoDS_Shape shell;
		if (!get_single_child(solid, shell)) {
			return false;
		}
		TopoDS_Shape face;
		if (!get_single_child(shell, face)) {
			return false;
		}
		TopoDS_Iterator it(face);
		return !it.More();
	}
}

bool OpenCascadeKernel::convert_impl(const taxonomy::boolean_result::ptr br, ConversionResults& results) {
	bool valid_result = false;
	bool first = true;
	const double tol = conv_settings_.getValue(ConversionSettings::GV_PRECISION);

	TopoDS_Shape a;
	TopTools_ListOfShape b;

	taxonomy::style::ptr first_item_style;

	for (auto& c : br->children) {
		IfcGeom::ConversionResults cr;
		AbstractKernel::convert(c, cr);
		if (first && br->operation == taxonomy::boolean_result::SUBTRACTION) {
			// @todo A will be null on union/intersection, intended?
			IfcGeom::util::flatten_shape_list(cr, a, false, conv_settings_.getValue(ifcopenshell::geometry::ConversionSettings::GV_PRECISION));
			first_item_style = c->surface_style;
			if (!first_item_style && c->kind() == taxonomy::COLLECTION) {
				// @todo recursively right?
				first_item_style = taxonomy::cast<taxonomy::geom_item>(taxonomy::cast<taxonomy::collection>(c)->children[0])->surface_style;
			}

			if (conv_settings_.getValue(ConversionSettings::GV_DISABLE_BOOLEAN_RESULT) > 0.0) {
				results.emplace_back(IfcGeom::ConversionResult(
					(int)br->instance->data().id(),
					br->matrix,
					new OpenCascadeShape(a),
					br->surface_style ? br->surface_style : first_item_style
				));
				return true;
			}

			const double first_operand_volume = util::shape_volume(a);
			if (first_operand_volume <= ALMOST_ZERO) {
				Logger::Message(Logger::LOG_WARNING, "Empty solid for:", c->instance);
			}
		} else {

			for (auto& r : cr) {
				auto S = ((OpenCascadeShape*)r.Shape())->shape();
				gp_GTrsf trsf;
				convert(r.Placement(), trsf);
				// @todo it really confuses me why I cannot use Moved() here instead
				S.Location(S.Location() * trsf.Trsf());

				if (is_unbounded_halfspace(S)) {
					double d;
					TopoDS_Shape result;
					util::fit_halfspace(a, S, result, d, tol * 1e3);
					// #2665 we also set a precision-independent treshold, because in the boolean op routine
					// the working fuzziness might still be increased.
					if (d < tol * 20. || d < 0.00002) {
						Logger::Message(Logger::LOG_WARNING, "Halfspace subtraction yields unchanged volume:", c->instance);
						continue;
					} else {
						S = result;
					}
				}

				b.Append(S);
			}
		}
		first = false;
	}

	util::boolean_settings bst;
	bst.attempt_2d = conv_settings_.getValue(ConversionSettings::GV_BOOLEAN_ATTEMPT_2D) > 0.;
	bst.debug = conv_settings_.getValue(ConversionSettings::GV_DEBUG_BOOLEAN) > 0.;
	bst.precision = conv_settings_.getValue(ConversionSettings::GV_PRECISION);

	TopoDS_Shape r;

	if (a.ShapeType() == TopAbs_COMPOUND && TopoDS_Iterator(a).More() && util::is_nested_compound_of_solid(a)) {
		TopoDS_Compound C;
		BRep_Builder B;
		B.MakeCompound(C);
		TopoDS_Iterator it(a);
		valid_result = true;
		for (; it.More(); it.Next()) {
			TopoDS_Shape part;
			if (util::boolean_operation(bst, it.Value(), b, op_to_occt(br->operation), part)) {
				B.Add(C, part);
			} else {
				valid_result = false;
			}
		}
		r = C;
	} else {
		valid_result = util::boolean_operation(bst, a, b, op_to_occt(br->operation), r);
	}

	results.emplace_back(IfcGeom::ConversionResult(
		(int) br->instance->data().id(),
		br->matrix,
		new OpenCascadeShape(r),
		br->surface_style ? br->surface_style : first_item_style
	));

	return valid_result;
}

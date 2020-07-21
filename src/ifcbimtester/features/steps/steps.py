import ifcopenshell
from behave import step

class IfcFile(object):
    file = None
    bookmarks = {}

    @classmethod
    def load(cls, path=None):
        cls.file = ifcopenshell.open(path)

    @classmethod
    def get(cls):
        if not cls.file:
            assert False, 'No file was loaded, so this requirement cannot be checked'
        return cls.file

    @classmethod
    def get_pset(cls, element, pset_name):
        if not element.IsDefinedBy:
            return
        for relationship in element.IsDefinedBy:
            if relationship.RelatingPropertyDefinition.Name == pset_name:
                return relationship.RelatingPropertyDefinition

    @classmethod
    def get_property(cls, element, pset_name, property_name):
        pset = cls.get_pset(element, pset_name)
        if not pset:
            return
        for prop in pset.HasProperties:
            if prop.Name == property_name:
                return prop


# Project setup MicroMVD


@step('The IFC file "{file}" must be provided')
def step_impl(context, file):
    try:
        IfcFile.load(file)
    except:
        assert False, f'The file {file} could not be loaded'


@step('IFC data must use the {schema} schema')
def step_impl(context, schema):
    assert IfcFile.get().schema == schema, \
        'We expected a schema of {} but instead got {}'.format(
            schema, IfcFile.get().schema)


@step('The IFC file "{file}" is exempt from being provided')
def step_impl(context, file):
    pass


@step('No further requirements are specified because {reason}')
def step_impl(context, reason):
    pass


@step('The project name, code, or short identifier must be "{value}"')
def step_impl(context, value):
    project_value = getattr(IfcFile.get().by_type('IfcProject')[0], 'Name')
    assert project_value == value, 'We expected a value of "{}" but instead got "{}"'.format(
        value, project_value)


@step('The project must have a longer form name of "{value}"')
def step_impl(context, value):
    project_value = getattr(IfcFile.get().by_type('IfcProject')[0], 'LongName')
    assert project_value == value, 'We expected a value of "{}" but instead got "{}"'.format(
        value, project_value)


@step('The project must be described as "{value}"')
def step_impl(context, value):
    project_value = getattr(IfcFile.get().by_type('IfcProject')[0], 'Description')
    assert project_value == value, 'We expected a value of "{}" but instead got "{}"'.format(
        value, project_value)


@step('The project must be categorised under "{value}"')
def step_impl(context, value):
    project_value = getattr(IfcFile.get().by_type('IfcProject')[0], 'ObjectType')
    assert project_value == value, 'We expected a value of "{}" but instead got "{}"'.format(
        value, project_value)


@step('The project must contain information about the "{value}" phase')
def step_impl(context, value):
    project_value = getattr(IfcFile.get().by_type('IfcProject')[0], 'Phase')
    assert project_value == value, 'We expected a value of "{}" but instead got "{}"'.format(
        value, project_value)


@step('The project must contain 3D geometry representing the shape of objects')
def step_impl(context):
    assert get_subcontext('Body', 'Model', 'MODEL_VIEW')


@step('The project must contain 3D geometry representing clearance zones')
def step_impl(context):
    assert get_subcontext('Clearance', 'Model', 'MODEL_VIEW')


@step('The project must contain 3D geometry representing the center of gravity of objects')
def step_impl(context):
    assert get_subcontext('CoG', 'Model', 'MODEL_VIEW')


@step('The project must contain 3D geometry representing the object bounding boxes')
def step_impl(context):
    assert get_subcontext('Box', 'Model', 'MODEL_VIEW')


def get_subcontext(identifier, type, target_view):
    project = IfcFile.get().by_type('IfcProject')[0]
    for rep_context in project.RepresentationContexts:
        for subcontext in rep_context.HasSubContexts:
            if subcontext.ContextIdentifier == identifier \
                    and subcontext.ContextType == type \
                    and subcontext.TargetView == target_view:
                return True
    assert False, 'The subcontext with identifier {}, type {}, and target view {} could not be found'.format(
        identifier, type, target_view)


# Custom MicroMVD

@step('The element {id} is an {ifc_class} only')
def step_impl(context, id, ifc_class):
    try:
        element = IfcFile.get().by_id(id)
    except:
        assert False, f'The element with {id} could not be found.'
    assert element.is_a() == ifc_class, 'The element {} is an {} instead of {}.'.format(element, element.is_a(), ifc_class)


@step('The element {id} is an {ifc_class}')
def step_impl(context, id, ifc_class):
    try:
        element = IfcFile.get().by_id(id)
    except:
        assert False, f'The element with {id} could not be found.'
    assert element.is_a(ifc_class), 'The element {} is an {} instead of {}.'.format(element, element.is_a(), ifc_class)


@step('The element {id} is further defined as a {predefined_type}')
def step_impl(context, id, predefined_type):
    try:
        element = IfcFile.get().by_id(id)
    except:
        assert False, f'The element with {id} could not be found.'
    if element.PredefinedType == predefined_type:
        return
    if element.PredefinedType == 'USERDEFINED' and element.ObjectType == 'predefined_type':
        return
    assert False, 'The element {} has a predefined type of {} and an object type of {} instead of {}.'.format(element, element.PredefinedType, element.ObjectType, predefined_type)


@step('The element {id} should not exist because {reason}')
def step_impl(context, id, reason):
    try:
        element = IfcFile.get().by_id(id)
    except:
        return
    assert False, 'This element {} should be reevaluated.'.format(element)


@step(u'there is at least one {ifc_class} element')
def step_impl(context, ifc_class):
    assert len(IfcFile.get().by_type(ifc_class)) >= 1


@step(u'there are no {ifc_class} elements because {reason}')
def step_impl(context, ifc_class, reason):
    assert len(IfcFile.get().by_type(ifc_class)) == 0


@step('all {ifc_class} elements have a name matching the pattern "{pattern}"')
def step_impl(context, ifc_class, pattern):
    import re
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        if not re.search(pattern, element.Name):
            assert False


@step('all {ifc_class} elements have an {representation_class} representation')
def step_impl(context, ifc_class, representation_class):
    def is_item_a_representation(item, representation):
        if '/' in representation:
            for cls in representation.split('/'):
                if item.is_a(cls):
                    return True
        elif item.is_a(representation):
            return True

    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        if not element.Representation:
            continue
        has_representation = False
        for representation in element.Representation.Representations:
            for item in representation.Items:
                if item.is_a('IfcMappedItem'):
                    # We only check one more level deep.
                    for item2 in item.MappingSource.MappedRepresentation.Items:
                        if is_item_a_representation(item2, representation_class):
                            has_representation = True
                else:
                    if is_item_a_representation(item, representation_class):
                        has_representation = True
        if not has_representation:
            assert False

use_step_matcher('re')
@step('all (?P<ifc_class>.*) elements have an? (?P<attribute>.*) attribute')
def step_impl(context, ifc_class, attribute):
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        if not getattr(element, attribute):
            assert False


@step('all (?P<ifc_class>.*) elements have an? (?P<property_path>.*\..*) property')
def step_impl(context, ifc_class, property_path):
    pset_name, property_name = property_path.split('.')
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        if not IfcFile.get_property(element, pset_name, property_name):
            assert False


@step('all (?P<ifc_class>.*) elements have an? (?P<property_path>.*\..*) property value matching the pattern "(?P<pattern>.*)"')
def step_impl(context, ifc_class, property_path, pattern):
    import re
    pset_name, property_name = property_path.split('.')
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        prop = IfcFile.get_property(element, pset_name, property_name)
        if not prop:
            assert False
        # For now, we only check single values
        if prop.is_a('IfcPropertySingleValue'):
            if not (prop.NominalValue \
                    and re.search(pattern, prop.NominalValue.wrappedValue)):
                assert False


@step('all (?P<ifc_class>.*) elements have an? (?P<attribute>.*) matching the pattern "(?P<pattern>.*)"')
def step_impl(context, ifc_class, attribute, pattern):
    import re
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        value = getattr(element, attribute)
        print(f'Checking value "{value}" for {element}')
        assert re.search(pattern, value)


@step('all (?P<ifc_class>.*) elements have an? (?P<attributes>.*) taken from the list in "(?P<list_file>.*)"')
def step_impl(context, ifc_class, attributes, list_file):
    import csv
    values = []
    with open(list_file) as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            values.append(row)
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        attribute_values = []
        for attribute in attributes.split(','):
            if not hasattr(element, attribute):
                assert False, f'Failed at element {element.GlobalId}'
            attribute_values.append(getattr(element, attribute))
        if attribute_values not in values:
            assert False, f'Failed at element {element.GlobalId}'


use_step_matcher('parse')
@step('all {ifc_class} elements have a {qto_name}.{quantity_name} quantity')
def step_impl(context, ifc_class, qto_name, quantity_name):
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        is_successful = False
        if not element.IsDefinedBy:
            assert False
        for relationship in element.IsDefinedBy:
            if relationship.RelatingPropertyDefinition.Name == qto_name:
                for quantity in relationship.RelatingPropertyDefinition.Quantities:
                    if quantity.Name == quantity_name:
                        is_successful = True
        if not is_successful:
            assert False

@step(u'the project should have geolocation data')
def step_impl(context):
    if IfcFile.get().schema == 'IFC2X3':
        for site in IfcFile.get().by_type('IfcSite'):
            if not IfcFile.get_pset(site, 'EPset_MapConversion'):
                assert False
    else:
        project = IfcFile.get().by_type('IfcProject')[0]
        for context in project.RepresentationContexts:
            if context.is_a('IfcGeometricRepresentationContext') \
                    and context.ContextType == 'Model' \
                    and context.HasCoordinateOperation:
                IfcFile.bookmarks['geolocation'] = context.HasCoordinateOperation[0].id()
                return
        assert False

@step(u'the project geolocation uses the "{crs_name}" CRS')
def step_impl(context, crs_name):
    if IfcFile.get().schema == 'IFC2X3':
        for site in IfcFile.get().by_type('IfcSite'):
            if IfcFile.get_property(site, 'EPset_ProjectedCRS', 'Name').NominalValue.wrappedValue != crs_name:
                assert False
    else:
        assert IfcFile.get().by_id(IfcFile.bookmarks['geolocation']).TargetCRS.Name == crs_name


use_step_matcher('re')
@step(u'the geolocated datum has an? (?P<attribute>.*) of "(?P<value>.*)"')
def step_impl(context, attribute, value):
    if IfcFile.get().schema == 'IFC2X3':
        site = IfcFile.get().by_type('IfcSite')[0]
        actual_value = IfcFile.get_property(site, 'EPset_MapConversion', attribute).NominalValue.wrappedValue
    else:
        actual_value = getattr(IfcFile.get().by_id(IfcFile.bookmarks['geolocation']), attribute)
    assert str(actual_value) == value, f'The value was {actual_value}'


use_step_matcher('parse')
@step(u'the project has a {attribute_name} attribute with a value of "{attribute_value}"')
def step_impl(context, attribute_name, attribute_value):
    project = IfcFile.get().by_type('IfcProject')[0]
    assert getattr(project, attribute_name) == attribute_value


@step(u'there is an {ifc_class} element with a {attribute_name} attribute with a value of "{attribute_value}"')
def step_impl(context, ifc_class, attribute_name, attribute_value):
    elements = IfcFile.get().by_type(ifc_class)
    for element in elements:
        if hasattr(element, attribute_name) \
                and getattr(element, attribute_name) == attribute_value:
            return
    assert False


@step(u'all buildings have an address')
def step_impl(context):
    for building in IfcFile.get().by_type('IfcBuilding'):
        if not building.BuildingAddress:
            assert False, f'The building "{building.Name}" has no address.'

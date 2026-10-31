"""Convert generated ISPA values into JSON without losing node metadata."""
import dataclasses
import enum


def to_json_value(value, seen=None):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, enum.Enum):
        return value.name
    seen = set() if seen is None else seen
    if id(value) in seen:
        return "<cycle>"
    seen.add(id(value))
    try:
        if isinstance(value, (list, tuple)):
            return [to_json_value(item, seen) for item in value]
        if isinstance(value, dict):
            return {str(key): to_json_value(item, seen) for key, item in value.items()}
        if dataclasses.is_dataclass(value):
            fields = {field.name: getattr(value, field.name) for field in dataclasses.fields(value)}
        elif hasattr(value, "__dict__"):
            fields = vars(value)
        else:
            fields = {name: getattr(value, name) for cls in type(value).__mro__
                      for name in getattr(cls, "__slots__", ()) if hasattr(value, name)}
        return {"$type": type(value).__qualname__, **{
            key: to_json_value(item, seen) for key, item in fields.items()
            if not key.startswith("_") and (not callable(item) or isinstance(item, (int, float, str)))}}
    finally:
        seen.remove(id(value))

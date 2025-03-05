BEGIN {
  inside_namespace = 0;
  inside_enum = 0;
}

# Detect the end of an enum declaration, and print ')' instead of '};'.
/^[[:space:]]+};/ {
  if (inside_enum)
    printf("\n)\n");
  inside_enum = 0;
  next;
}

# Don't print the opening brace of an enum declaration.
/^[[:space:]]+{/ {
  next;
}

# But print everything else inside an enum declaration.
{
  if (inside_enum)
  {
    if (saw_equals)
    {
      if ($0 ~ /,/)
        need_comma = 1;
      else
        need_comma = 0;
      printf("\n");
      saw_equals = 0;
    }
    else if ($0 ~ /^#/)
    {
      printf("\n%s", $0);
      saw_directive = 1;
    }
    else
    {
      if ($0 ~ /=$/)
      {
        enum_name = gensub(/\s*(\w+)\s*=/, "\\1", "g")
        if (need_comma)
          printf(",\n");
        printf("  %s::%s", enum_type, enum_name);
        saw_equals = 1;
      }
      else
      {
        enum_name = gensub(/\s*(\w+)\s*(VULKAN_HPP_DEPRECATED_[0-9]+\(\s*"[^"]*"\s*\))?\s*(=[^,]*)?(,)?\s*/, "\\1", "g")
        if (need_comma)
        {
          if (saw_directive)
            printf("\n  ,");
          else
            printf(",\n");
        }
        printf("  %s::%s", enum_type, enum_name);
        comma = gensub(/\s*(\w+)\s*(VULKAN_HPP_DEPRECATED_[0-9]+\(\s*"[^"]*"\s*\))?\s*(=[^,]*)?(,)?\s*/, "\\4", "g")
        if (comma == ",")
          need_comma = 1;
        else
          need_comma = 0;
      }
      saw_directive = 0;
    }
    next;
  }
}

# Everything below is for lines outside of an enum declaration.

# Keep track of whether or not we're inside the namespace.
/^{/ {
  inside_namespace = 1;
  next;
}

/^}/ {
  inside_namespace = 0;
  next;
}

# Print all conditional preprocessing directive.
/^#(if|else|elif|endif)/ {
  if (inside_namespace)
    print;
  next;
}

# Detect the start of an enum declaration and morph it into a macro.
/^[[:space:]]*enum class/ {
  inside_enum = 1;
  comma = "\n  ";
  if ($0 ~ /^\s*enum class\s+(\w+)\s*(:\s*(\w+))?/) {
    enum_type = gensub(/^\s*enum class\s+(\w+)\s*(:\s*(\w+))?/, "\\1", "g")
    enum_underlying_type = gensub(/^\s*enum class\s+(\w+)\s*(:\s*(\w+))?/, "\\3", "g")
    if (enum_underlying_type == "")
      enum_underlying_type = "int";
    printf("ENUM_DECLARATION(%s, %s,\n", enum_type, enum_underlying_type)
  }
  next;
}

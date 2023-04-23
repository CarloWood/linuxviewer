// A function to insert new tokens before a given token in the nested
// highlighting property 'inside' of the prism syntax highlighting rules.
function insertInsideBefore(language, parentToken, beforeToken, newTokens) {
  const parent = Prism.languages[language][parentToken].inside;
  const newParentInside = {};

  for (const token in parent) {
    if (token === beforeToken) {
      for (const newKey in newTokens) {
        newParentInside[newKey] = newTokens[newKey];
      }
    }
    newParentInside[token] = parent[token];
  }

  Prism.languages[language][parentToken].inside = newParentInside;
}

// Replace the 'string' token of the cpp.macro.inside sub-pattern property with 'directive-included'.
insertInsideBefore('cpp', 'macro', 'string', {
  'directive-included': [
    {
      pattern: /^(#\s*include\s*)<[^>]+>/,
      lookbehind: true
    },
    Prism.languages.cpp.string
  ],
  'directive-include': [
    {
      pattern: /^#\s*include\b/,
    },
  ],
});

// Delete the 'string' token because we won't use it anymore anyway.
delete Prism.languages.cpp.macro.inside.string;

// Insert a token 'namespace-vulkan' to match the vulkan namespace at the beginning of the 'inside' property of 'base-clause'.
Prism.languages.cpp['base-clause'].inside = {
  'namespace-vulkan-nested': {
    pattern: /\b(vulkan::)(\w+(?=::))/,
    lookbehind: true
  },
  'namespace-vulkan': {
    pattern: /\bvulkan(?=::)/,
  },
  'other-namespace': {
    pattern: /\b\w+(?=::)/,
    alias: 'namespace',
  },
  'cpp-access': {
    pattern: /\b(public|protected|private)\b/
  },
  ...Prism.languages.cpp['base-clause'].inside
};

// Insert a token 'pseudo-statement' at the beginning of the 'inside' property of 'generic-function'.
Prism.languages.cpp['generic-function'].inside = {
  'pseudo-statement': {
    pattern: /\b(return|const_cast|static_cast|dynamic_cast|reinterpret_cast)\b/,
  },
  ...Prism.languages.cpp['generic-function'].inside
};

Prism.languages.insertBefore('cpp', 'macro', {
  'macro-name': {
    pattern: /\b(CWDEBUG|LAYOUT_DECLARATION|LAYOUT)\b/
  }
});

Prism.languages.insertBefore('cpp', 'class-name', {
  'function': {
    pattern: /\b[a-z0-9_]+(?=(<.*>)?\()/,
  },
  'cpp-modifier': {
    pattern: /\b(inline|virtual|explicit|export|override|final)\b/
  },
  'structure': {
    pattern: /\b(class|struct|enum)\b/
  },
  'cpp-access': {
    pattern: /\b(public|protected|private)\b/
  },
  'debug-macro': {
    pattern: /\b(Debug|LibcwDebug|DoutFatal|DoutEntering|Dout|LibcwDoutFatal|LibcwDout|ASSERT|assert|DEBUG_ONLY|CWDEBUG_ONLY|COMMA_CWDEBUG_ONLY|NEW|AllocTag[12]?|AllocTag_dynamic_description|ForAllDebugChannels|ForAllDebugObjects|NAMESPACE_DEBUG_CHANNELS_START|NAMESPACE_DEBUG_CHANNELS_END|NAMESPACE_DEBUG|NAMESPACE_DEBUG_START|NAMESPACE_DEBUG_END|DEBUGCHANNELS)\b/
  },
  'builtin-type': {
    pattern: /\b(int|long|short|char|void|signed|unsigned|float|double|size_t|ssize_t|off_t|wchar_t|ptrdiff_t|sig_atomic_t|fpos_t|clock_t|time_t|va_list|jmp_buf|FILE|DIR|div_t|ldiv_t|mbstate_t|wctrans_t|wint_t|wctype_t|_Bool|bool|_Complex|complex|_Imaginary|imaginary|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t|int_least8_t|int_least16_t|int_least32_t|int_least64_t|uint_least8_t|uint_least16_t|uint_least32_t|uint_least64_t|int_fast8_t|int_fast16_t|int_fast32_t|int_fast64_t|uint_fast8_t|uint_fast16_t|uint_fast32_t|uint_fast64_t|intptr_t|uintptr_t|intmax_t|uintmax_t|__label__|__complex__|__volatile__|char16_t|char32_t)\b/
  },
  'std-container': {
    pattern: /(\bstd::)(vector|list|map|set|multimap|multiset)\b/,
    lookbehind: true,
    alias: 'class-name',
  },
  'class-name-vulkan-nested': {
    pattern: /(\bvulkan::)([A-Z][A-Za-z]+)\b/,
    lookbehind: true,
    alias: 'class-name'
  },
  'example-class-name': {
    pattern: /\b(Triangle\w*|HelloTriangle|Window|SynchronousWindow|RenderPass|Attachment|LogicalDevice|VertexData)\b/,
    alias: 'class-name'
  },
  'class-name-std-nested': {
    pattern: /\b(std::)([a-z0-9]+)\b/,
    lookbehind: true,
    alias: 'class-name'
  },
  'namespace-std': {
    pattern: /\bstd(?=::)/,
    alias: 'namespace',
  },
  'namespace-vk-type': {
    pattern: /\b(vk::)(\w+)/,
    lookbehind: true,
  },
  'namespace-vk': {
    pattern: /\bvk(?=::)/,
    alias: 'namespace',
  },
  'namespace-vulkan-nested': {
    pattern: /\b(vulkan::)(\w+(?=::))/,
    lookbehind: true
  },
  'namespace-vulkan': {
    pattern: /\bvulkan(?=::)/,
  },
  'other-namespace': {
    pattern: /\b\w+(?=::)/,
    alias: 'namespace',
  },
  'enum': {
    pattern: /(::)(e[A-Z]\w+)/,
    lookbehind: true,
  },
  'nested-class-name': {
    pattern: /(::)([A-Z]\w+)/,
    lookbehind: true,
    alias: 'class-name'
  },
  'member-variable': {
    pattern: /\b(m_\w+|s_\w+|root_window_request_cookie|main_pass|depth)\b/,
    alias: 'variable',
  },
  'pseudo-statement': {
    pattern: /\b(return|const_cast|static_cast|dynamic_cast|reinterpret_cast)\b/,
  },
});

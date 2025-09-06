# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

import os
import sys
sys.path.insert(0, os.path.abspath('../src'))

# -- Project information -----------------------------------------------------

project = 'Gaussian Extractor'
copyright = '2025, Le Nhan Pham'
author = 'Le Nhan Pham'
release = '0.5.0'
version = '0.5.0'

# -- General configuration ---------------------------------------------------

# Master document (moved here to be defined before usage)
master_doc = 'index'

# Add any Sphinx extension module names here, as strings.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.doctest',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.coverage',
    'sphinx.ext.mathjax',
    'sphinx.ext.ifconfig',
    'sphinx.ext.viewcode',
    'sphinx.ext.githubpages',
    'sphinx.ext.napoleon',
    'breathe',
    'exhale',  
]

exhale_args = {
    "containmentFolder": "./api",
    "rootFileName": "api.rst",
    "rootFileTitle": "Gaussian Extractor API",
    "doxygenStripFromPath": os.path.abspath("../src"),
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin": """
        PROJECT_NAME = GaussianExtractor
        INPUT = ../src
        FILE_PATTERNS = *.cpp *.hpp *.h *.cxx *.cc
        RECURSIVE = YES
        GENERATE_XML = YES
        XML_OUTPUT = xml
        EXTRACT_ALL = YES
        EXTRACT_PRIVATE = YES
        EXTRACT_STATIC = YES
        EXTRACT_LOCAL_CLASSES = YES
        EXTRACT_LOCAL_METHODS = YES
        EXTRACT_ANON_NSPACES = YES
        ENABLE_PREPROCESSING = YES
        VERBATIM_HEADERS = NO
        XML_PROGRAMLISTING = NO
        MACRO_EXPANSION = YES
        EXPAND_ONLY_PREDEF = YES
        INLINE_SOURCES = YES
        SHOW_FILES = YES
        EXCLUDE_PATTERNS = */test/* */tests/* */example/* */examples/*
        HAVE_DOT = YES
        UML_LOOK = YES
        CALL_GRAPH = YES
        CALLER_GRAPH = YES
        INCLUDE_GRAPH = YES
        INCLUDED_BY_GRAPH = YES
        COLLABORATION_GRAPH = YES
        CLASS_DIAGRAMS = YES
        CLASS_GRAPH = YES
        GRAPHICAL_HIERARCHY = YES
        DIRECTORY_GRAPH = YES
        DOT_IMAGE_FORMAT = svg
        INTERACTIVE_SVG = YES
        DOT_TRANSPARENT = YES
        DOT_MULTI_TARGETS = YES
        QUIET = YES
        EXCLUDE_PATTERNS = */src/*.cpp
    """,
    "createTreeView": True,
    "afterTitleDescription": "Comprehensive API documentation for the Gaussian Extractor.",
    "verboseBuild": False,
}



breathe_projects = {
    'GaussianExtractor': './xml/'  # Path to Doxygen XML output
}
breathe_default_project = 'GaussianExtractor'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------

html_theme = "furo"

# Add any paths that contain custom static files.
html_static_path = ['_static']

# Remove custom CSS/JS unless files exist in _static/
html_css_files = []
html_js_files = []

# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    'papersize': 'a4paper',
    'pointsize': '11pt',
    'preamble': r'''
        \usepackage{amsmath}
        \usepackage{amssymb}
        \usepackage{physics}
        \usepackage{siunitx}
    ''',
    'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files.
latex_documents = [
    (master_doc, 'GaussianExtractor.tex', 'Gaussian Extractor Documentation',
     'Le Nhan Pham', 'manual'),
]

# -- Options for manual page output ------------------------------------------

man_pages = [
    (master_doc, 'gaussianextractor', 'Gaussian Extractor Documentation',
     [author], 1)
]

# -- Options for Texinfo output ----------------------------------------------

texinfo_documents = [
    (master_doc, 'GaussianExtractor', 'Gaussian Extractor Documentation',
     author, 'GaussianExtractor', 'High-performance Gaussian log file processor.',
     'Miscellaneous'),
]

# -- Extension configuration --------------------------------------------------

# Intersphinx mapping
intersphinx_mapping = {
    'python': ('https://docs.python.org/3/', None),
}

# If true, `todo` and `todoList` produce output.
todo_include_todos = True

# Autodoc configuration
autoclass_content = 'both'
autodoc_member_order = 'bysource'
autodoc_default_flags = ['members', 'undoc-members', 'show-inheritance']

# Napoleon configuration for Google/NumPy style docstrings
napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = False
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = False
napoleon_use_admonition_for_notes = False
napoleon_use_admonition_for_references = False
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True

# Pygments style for syntax highlighting
pygments_style = 'sphinx'

# Keep warnings as system messages
keep_warnings = False

# -- GitHub integration -----------------------------------------------------

html_context = {
    'display_github': True,
    'github_user': 'lenhanpham',
    'github_repo': 'gaussian-extractor',
    'github_version': 'main',
    'conf_py_path': '/docs/',
}

# -- PDF output configuration -----------------------------------------------

pdf_documents = [
    (master_doc, 'GaussianExtractor', 'Gaussian Extractor Documentation',
     author, 'manual'),
]

# -- Linkcheck configuration ------------------------------------------------

linkcheck_ignore = [
    r'https://github.com/lenhanpham/gaussian-extractor/.*',
    r'https://docs.python.org/.*',
    r'https://en.cppreference.com/.*',
]

# -- Custom build commands --------------------------------------------------

def setup(app):
    """Custom setup function for additional configurations."""
    return {
        'version': '0.1',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
set(classes
  pqAnimationKeyFrame
  pqAnimationModel
  pqAnimationTrack
  pqAnimationWidget
  pqCheckBoxPixMaps
  pqCheckBoxPixMaps
  pqCheckableHeaderView
  pqClickableLabel
  pqCollapsedGroup
  pqColorButtonEventPlayer
  pqColorButtonEventTranslator
  pqColorChooserButton
  pqColorDialogEventPlayer
  pqColorDialogEventTranslator
  pqConnect
  pqConsoleWidget
  pqConsoleWidgetEventPlayer
  pqConsoleWidgetEventTranslator
  pqDelimitedTextParser
  pqDiscreteDoubleWidget
  pqDoubleLineEdit
  pqDoubleRangeDialog
  pqDoubleRangeWidget
  pqDoubleSliderWidget
  pqDoubleSpinBox
  pqExpandableTableView
  pqExpanderButton
  pqFlatTreeView
  pqHeaderView
  pqHierarchicalGridLayout
  pqHierarchicalGridWidget
  pqHighlightablePushButton
  pqHighlightableToolButton
  pqLineEdit
  pqLineEditEventPlayer
  pqListWidgetCheckHelper
  pqListWidgetItemObject
  pqMultiColumnHeaderView
  pqOneLinerTextEdit
  pqProgressWidget
  pqQuickLaunchDialog
  pqScaleByButton
  pqScaledSpinBox
  pqSectionVisibilityContextMenu
  pqSeriesGeneratorDialog
  pqSetData
  pqSetName
  pqSignalAdaptors
  pqSpinBox
  pqTableView
  pqTextEdit
  pqTimelineScrollbar
  pqTreeView
  pqTreeViewExpandState
  pqTreeViewSelectionHelper
  pqTreeWidget
  pqTreeWidgetCheckHelper
  pqTreeWidgetItem
  pqTreeWidgetItemObject
  pqTreeWidgetSelectionHelper
  pqVectorWidget
  pqWaitCursor
  pqWidgetsInit)

set(headers)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pqQtWidgetsConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pqQtWidgetsConfig.h"
  @ONLY)
list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/pqQtWidgetsConfig.h")

set(qt_components
  Core
  Gui
  Network
  Widgets)

if (APPLE)
  list(APPEND qt_components
    PrintSupport)
endif ()

if (PARAVIEW_USE_QTHELP)
  list(APPEND classes
    pqHelpWindow)

  list(APPEND qt_components
    Help)
endif ()

if (PARAVIEW_USE_QTWEBENGINE)
  list(APPEND qt_components
    WebEngineWidgets)
endif ()

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  ${qt_components})

##########################################################################
## Add *.ui files to be uic processed.
set(ui_files
  Resources/UI/pqExpanderButton.ui
  Resources/UI/pqHelpWindow.ui
  Resources/UI/pqQuickLaunchDialog.ui
  Resources/UI/pqSeriesGeneratorDialog.ui)

##########################################################################
## Add resources.
set(resource_files
  Resources/QtWidgets.qrc)

source_group("Resources" FILES
  ${ui_files}
  ${resource_files})

list(APPEND headers
  pqQtDeprecated.h
)

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTORCC 1)
set(CMAKE_AUTOUIC 1)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/UI")

vtk_module_add_module(ParaView::pqWidgets
  CLASSES ${classes}
  SOURCES ${ui_files}
          ${resource_files}
  HEADERS ${headers})
vtk_module_link(ParaView::pqWidgets
  PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
  PRIVATE
    Qt5::Network)
vtk_module_definitions(ParaView::pqWidgets PRIVATE QT_NO_KEYWORDS)

if (APPLE)
  vtk_module_link(ParaView::pqWidgets
    PRIVATE
      Qt5::PrintSupport)
endif ()

if (PARAVIEW_USE_QTHELP)
  vtk_module_link(ParaView::pqWidgets
    PRIVATE
      Qt5::Help)
endif ()

if (PARAVIEW_USE_QTWEBENGINE)
  vtk_module_link(ParaView::pqWidgets
    PRIVATE
      Qt5::WebEngineWidgets)
endif ()

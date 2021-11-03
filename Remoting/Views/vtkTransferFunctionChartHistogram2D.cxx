/*=========================================================================

  Program:   ParaView
  Module:    vtkTransferFunctionChartHistogram2D.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionChartHistogram2D.h"

// paraview includes
#include "vtkPVDiscretizableColorTransferFunction.h"
#include "vtkTransferFunctionBoxItem.h"

// vtk includes
#include "vtkAxis.h"
#include "vtkContextKeyEvent.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPVTransferFunction2D.h"
#include "vtkPlotHistogram2D.h"
#include "vtkPointData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStringArray.h"

//-------------------------------------------------------------------------------------------------
vtkStandardNewMacro(vtkTransferFunctionChartHistogram2D);

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionChartHistogram2D::IsInitialized()
{
  auto const plot = vtkPlotHistogram2D::SafeDownCast(this->GetPlot(0));
  if (!plot)
  {
    return false;
  }
  if (plot->GetInputImageData())
  {
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionChartHistogram2D::MouseDoubleClickEvent(const vtkContextMouseEvent& mouse)
{
  if (this->IsInitialized())
  {
    this->AddNewBox();
  }
  return Superclass::MouseDoubleClickEvent(mouse);
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkTransferFunctionBoxItem> vtkTransferFunctionChartHistogram2D::AddNewBox()
{
  double xRange[2];
  auto bottomAxis = this->GetAxis(vtkAxis::BOTTOM);
  bottomAxis->GetRange(xRange);

  double yRange[2];
  auto leftAxis = this->GetAxis(vtkAxis::LEFT);
  leftAxis->GetRange(yRange);

  const double width = (xRange[1] - xRange[0]) / 3.0;
  const double height = (yRange[1] - yRange[0]) / 3.0;
  vtkRectd box(xRange[0] + width, yRange[0] + height, width, height);
  double color[3] = { 1, 0, 0 };
  double alpha = 1.0;
  auto boxItem = this->AddNewBox(box, color, alpha);
  this->SetActiveBox(boxItem);
  //  if (this->Transfer2DBoxesItem)
  //  {
  //    this->Transfer2DBoxesItem->AddTransfer2DBox(boxItem);
  //  }
  return boxItem;
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkTransferFunctionBoxItem> vtkTransferFunctionChartHistogram2D::AddNewBox(
  const vtkRectd& box, double* color, double alpha, bool addToTF2D)
{
  if (!this->IsInitialized())
  {
    return nullptr;
  }

  double xRange[2];
  auto bottomAxis = this->GetAxis(vtkAxis::BOTTOM);
  bottomAxis->GetRange(xRange);

  double yRange[2];
  auto leftAxis = this->GetAxis(vtkAxis::LEFT);
  leftAxis->GetRange(yRange);

  vtkNew<vtkTransferFunctionBoxItem> boxItem;
  // Set bounds in the box item so that it can only move within the
  // histogram's range.
  boxItem->SetValidBounds(xRange[0], xRange[1], yRange[0], yRange[1]);
  boxItem->SetBox(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight());
  boxItem->SetBoxColor(color[0], color[1], color[2], alpha);
  this->AddBox(boxItem, addToTF2D);
  return boxItem;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::AddBox(
  vtkSmartPointer<vtkTransferFunctionBoxItem> box, bool addToTF2D)
{
  if (!this->IsInitialized() || !box)
  {
    return;
  }
  // Check if the chart already contains the box
  if (this->GetPlotIndex(box) > -1)
  {
    // Plot index other than -1 indicates the chart already has the box
    return;
  }

  // Add the observer to update the transfer function on interaction
  if (addToTF2D)
  {
    box->AddObserver(vtkTransferFunctionBoxItem::BoxAddEvent, this,
      &vtkTransferFunctionChartHistogram2D::OnTransferFunctionBoxItemModified);
  }
  box->AddObserver(vtkTransferFunctionBoxItem::BoxEditEvent, this,
    &vtkTransferFunctionChartHistogram2D::OnTransferFunctionBoxItemModified);
  box->AddObserver(vtkTransferFunctionBoxItem::BoxSelectEvent, this,
    &vtkTransferFunctionChartHistogram2D::OnTransferFunctionBoxItemModified);
  box->AddObserver(vtkTransferFunctionBoxItem::BoxDeleteEvent, this,
    &vtkTransferFunctionChartHistogram2D::OnTransferFunctionBoxItemModified);
  this->AddPlot(box);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::SetInputData(vtkImageData* data, vtkIdType z)
{
  if (data)
  {
    int bins[3];
    double origin[3], spacing[3];
    data->GetOrigin(origin);
    data->GetDimensions(bins);
    data->GetSpacing(spacing);

    // Compute image bounds
    const double xMin = origin[0];
    const double xMax = origin[0] + bins[0] * spacing[0];
    const double yMin = origin[1];
    const double yMax = origin[1] + bins[1] * spacing[1];

    auto axis = GetAxis(vtkAxis::BOTTOM);
    axis->SetUnscaledRange(xMin, xMax);
    axis = GetAxis(vtkAxis::LEFT);
    axis->SetUnscaledRange(yMin, yMax);
    this->RecalculatePlotTransforms();

    UpdateItemsBounds(xMin, xMax, yMin, yMax);

    // Set the histogram and initialize the chart
    this->Superclass::SetInputData(data, z);

    // Now add the transfer function boxes from the color function
    //    if (this->Transfer2DBoxesItem)
    //    {
    //      std::vector<vtkSmartPointer<vtkTransferFunctionBoxItem>> boxes =
    //        this->Transfer2DBoxesItem->GetTransferFunction2DBoxes();
    //      for (auto it = boxes.begin(); it < boxes.end(); ++it)
    //      {
    //        vtkSmartPointer<vtkTransferFunctionBoxItem> box = *(it);
    //        if (!box)
    //        {
    //          continue;
    //        }
    //        box->SetValidBounds(xMin, xMax, yMin, yMax);
    //        this->AddBox(box);
    //      }
    //    }

    if (auto arr =
          vtkStringArray::SafeDownCast(data->GetFieldData()->GetAbstractArray("ArrayNames")))
    {
      this->GetAxis(vtkAxis::BOTTOM)->SetTitle(arr->GetValue(0));
      this->GetAxis(vtkAxis::LEFT)->SetTitle(arr->GetValue(1));
    }

    if (this->TransferFunction2D)
    {
      auto boxes = this->TransferFunction2D->GetBoxes();
      for (auto it = boxes.cbegin(); it != boxes.cend(); ++it)
      {
        auto const b = (*it);
        double* c = b->GetColor();
        // Add a box item to the chart based on existing boxes in the transfer function
        // making sure that the box is not duplicated in the function (addToTF2D = false).
        auto bItem = this->AddNewBox(b->GetBox(), c, c[3], false);
        bItem->SetID(std::distance(boxes.cbegin(), it));
      }
      this->GenerateTransfer2D();
    }
  }
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::SetTransferFunction2D(vtkPVTransferFunction2D* transfer2D)
{
  this->TransferFunction2D = transfer2D;
}

//-------------------------------------------------------------------------------------------------
vtkPVTransferFunction2D* vtkTransferFunctionChartHistogram2D::GetTransferFunction2D()
{
  return this->TransferFunction2D.GetPointer();
}

////-------------------------------------------------------------------------------------------------
// void vtkTransferFunctionChartHistogram2D::SetTransfer2DBoxesItem(
//  vtkPVDiscretizableColorTransferFunction* t2dBoxes)
//{
//  this->Transfer2DBoxesItem = t2dBoxes;
//}
//
////-------------------------------------------------------------------------------------------------
// vtkPVDiscretizableColorTransferFunction*
// vtkTransferFunctionChartHistogram2D::GetTransfer2DBoxesItem()
//{
//  return this->Transfer2DBoxesItem;
//}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::UpdateItemsBounds(
  const double xMin, const double xMax, const double yMin, const double yMax)
{
  // Set the new bounds to its current box items (plots).
  const vtkIdType numPlots = GetNumberOfPlots();
  for (vtkIdType i = 0; i < numPlots; i++)
  {
    auto boxItem = vtkControlPointsItem::SafeDownCast(GetPlot(i));
    if (!boxItem)
    {
      continue;
    }

    boxItem->SetValidBounds(xMin, xMax, yMin, yMax);
  }
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::GenerateTransfer2D()
{
  if (!this->IsInitialized())
  {
    return;
  }
  // const vtkIdType numPlots = this->GetNumberOfPlots();

  vtkSmartPointer<vtkImageData> histogram =
    vtkPlotHistogram2D::SafeDownCast(this->GetPlot(0))->GetInputImageData();
  if (!histogram)
  {
    return;
  }

  if (!this->TransferFunction2D)
  {
    return;
  }

  int dims[3];
  histogram->GetDimensions(dims);
  this->TransferFunction2D->SetOutputDimensions(dims[0], dims[1]);
  this->TransferFunction2D->Build();

  //  if (auto oldScalars = this->TransferFunction2D->GetPointData()->GetScalars())
  //  {
  //    this->TransferFunction2D->GetPointData()->RemoveArray(oldScalars->GetName());
  //  }
  //
  //  double spacing[3];
  //  histogram->GetSpacing(spacing);
  //  double origin[3];
  //  histogram->GetOrigin(origin);
  //  int dims[3];
  //  histogram->GetDimensions(dims);
  //
  //  this->TransferFunction2D->SetDimensions(dims);
  //  this->TransferFunction2D->SetOrigin(origin);
  //  this->TransferFunction2D->SetSpacing(spacing);
  //  this->TransferFunction2D->AllocateScalars(VTK_FLOAT, 4);
  //  auto arr =
  //  vtkFloatArray::SafeDownCast(this->TransferFunction2D->GetPointData()->GetScalars()); auto
  //  arrRange = vtk::DataArrayValueRange(arr); std::fill(arrRange.begin(), arrRange.end(), 0.0);
  //
  //  for (vtkIdType i = 0; i < numPlots; ++i)
  //  {
  //    auto boxItem = vtkTransferFunctionBoxItem::SafeDownCast(this->GetPlot(i));
  //    if (!boxItem)
  //    {
  //      continue;
  //    }
  //    const vtkRectd box = boxItem->GetBox();
  //    vtkIdType width = static_cast<vtkIdType>(box.GetWidth() / spacing[0]);
  //    vtkIdType height = static_cast<vtkIdType>(box.GetHeight() / spacing[1]);
  //    if (width <= 0 || height <= 0)
  //    {
  //      continue;
  //    }
  //    vtkIdType x0 = static_cast<vtkIdType>((box.GetX() - origin[0]) / spacing[0]);
  //    vtkIdType y0 = static_cast<vtkIdType>((box.GetY() - origin[1]) / spacing[1]);
  //    vtkSmartPointer<vtkImageData> boxTexture = boxItem->GetTexture();
  //    int boxDims[3];
  //    boxTexture->GetDimensions(boxDims);
  //    double boxSpacing[3] = { 1, 1, 1 };
  //    boxSpacing[0] = box.GetWidth() / boxDims[0];
  //    boxSpacing[1] = box.GetHeight() / boxDims[1];
  //    for (vtkIdType ii = x0; ii < x0 + width; ++ii)
  //    {
  //      for (vtkIdType jj = y0; jj < y0 + height; ++jj)
  //      {
  //        int boxCoord[3] = { 0, 0, 0 };
  //        boxCoord[0] = (ii - x0) * spacing[0] / boxSpacing[0];
  //        boxCoord[1] = (jj - y0) * spacing[1] / boxSpacing[1];
  //        unsigned char* ptr = static_cast<unsigned
  //        char*>(boxTexture->GetScalarPointer(boxCoord)); float fptr[4]; for (int tp = 0; tp < 4;
  //        ++tp)
  //        {
  //          fptr[tp] = ptr[tp] / 255.0;
  //        }
  //        // composite this color with the current color
  //        float* c = static_cast<float*>(this->TransferFunction2D->GetScalarPointer(ii, jj, 0));
  //        float opacity = c[3] + fptr[3] * (1 - c[3]);
  //        opacity = opacity > 1.0 ? 1.0 : opacity;
  //        for (int tp = 0; tp < 3; ++tp)
  //        {
  //          c[tp] = (fptr[tp] * fptr[3] + c[tp] * c[3] * (1 - fptr[tp])) / opacity;
  //          c[tp] = c[tp] > 1.0 ? 1.0 : c[tp];
  //        }
  //        c[3] = opacity;
  //      }
  //    }
  //  }
  this->InvokeEvent(vtkTransferFunctionChartHistogram2D::TransferFunctionModified, nullptr);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::OnTransferFunctionBoxItemModified(
  vtkObject* caller, unsigned long eid, void* vtkNotUsed(callData))
{
  auto plot = reinterpret_cast<vtkTransferFunctionBoxItem*>(caller);
  if (!plot)
  {
    return;
  }
  switch (eid)
  {
    case vtkTransferFunctionBoxItem::BoxSelectEvent:
    {
      this->SetActiveBox(plot);
      break;
    }
    case vtkTransferFunctionBoxItem::BoxAddEvent:
    {
      if (this->TransferFunction2D)
      {
        auto box = plot->GetTransferFunctionBox();
        plot->SetID(this->TransferFunction2D->AddControlBox(box));
        // this->TransferFunction2D->AddControlBox(plot->GetTransferFunctionBox());
        this->GenerateTransfer2D();
      }
      break;
    }
    case vtkTransferFunctionBoxItem::BoxEditEvent:
    {
      plot->SetID(
        this->TransferFunction2D->SetControlBox(plot->GetID(), plot->GetTransferFunctionBox()));
      this->GenerateTransfer2D();
      break;
    }
    case vtkTransferFunctionBoxItem::BoxDeleteEvent:
    {
      auto boxItem = this->GetActiveBox();
      if (boxItem)
      {
        this->RemoveBox(boxItem);
      }
      break;
    }
    default:
    {
      break;
    }
  }
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkTransferFunctionBoxItem> vtkTransferFunctionChartHistogram2D::GetActiveBox()
  const
{
  return this->ActiveBox;
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::SetActiveBox(
  vtkSmartPointer<vtkTransferFunctionBoxItem> box)
{
  if (this->ActiveBox)
  {
    this->ActiveBox->SetSelected(false);
    this->ActiveBox->SetCurrentPoint(-1);
  }
  this->ActiveBox = box;
  if (this->ActiveBox)
  {
    this->ActiveBox->SetSelected(true);
  }
  this->GetScene()->SetDirty(true);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::SetActiveBoxColorAlpha(
  double r, double g, double b, double a)
{
  if (!this->ActiveBox)
  {
    return;
  }
  this->ActiveBox->SetBoxColor(r, g, b, a);
  this->GetScene()->SetDirty(true);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::SetActiveBoxColorAlpha(double color[3], double alpha)
{
  this->SetActiveBoxColorAlpha(color[0], color[1], color[2], alpha);
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionChartHistogram2D::KeyPressEvent(const vtkContextKeyEvent& key)
{
  if (key.GetInteractor()->GetKeySym() == std::string("Delete") ||
    key.GetInteractor()->GetKeySym() == std::string("BackSpace"))
  {
    auto boxItem = this->GetActiveBox();
    if (boxItem)
    {
      this->RemoveBox(boxItem);
    }
    return true;
  }
  return this->Superclass::KeyPressEvent(key);
}

//-------------------------------------------------------------------------------------------------
void vtkTransferFunctionChartHistogram2D::RemoveBox(vtkSmartPointer<vtkTransferFunctionBoxItem> box)
{
  if (!box)
  {
    return;
  }

  const vtkIdType numPlots = GetNumberOfPlots();
  for (vtkIdType i = 0; i < numPlots; i++)
  {
    auto boxItem = vtkControlPointsItem::SafeDownCast(GetPlot(i));
    if (!boxItem)
    {
      continue;
    }
    if (boxItem == box)
    {
      this->BoxesToRemove.push_back(i);
      this->GetScene()->SetDirty(true);
      break;
    }
  }
  if (this->ActiveBox == box)
  {
    this->ActiveBox = nullptr;
  }
  if (this->TransferFunction2D)
  {
    this->TransferFunction2D->RemoveControlBox(box->GetID());
  }
}

//-------------------------------------------------------------------------------------------------
bool vtkTransferFunctionChartHistogram2D::Paint(vtkContext2D* painter)
{
  if (!this->BoxesToRemove.empty())
  {
    for (auto i = this->BoxesToRemove.cbegin(); i < this->BoxesToRemove.cend(); ++i)
    {
      this->RemovePlot(*i);
    }
    this->BoxesToRemove.clear();
    this->GenerateTransfer2D();
  }
  return Superclass::Paint(painter);
}

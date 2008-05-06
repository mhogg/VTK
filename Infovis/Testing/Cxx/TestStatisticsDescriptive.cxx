/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkDoubleArray.h"
#include "vtkTable.h"
#include "vtkStatisticsDescriptive.h"

//=============================================================================
int TestStatisticsDescriptive( int, char *[] )
{
  int testIntValue = 0;

  double mingledData[] = 
    {
    46,
    45,
    47,
    49,
    46,
    47,
    46,
    46,
    47,
    46,
    47,
    49,
    49,
    49,
    47,
    45,
    50,
    50,
    46,
    46,
    51,
    50,
    48,
    48,
    52,
    54,
    48,
    47,
    52,
    52,
    49,
    49,
    53,
    54,
    50,
    50,
    53,
    54,
    50,
    52,
    53,
    53,
    50,
    51,
    54,
    54,
    49,
    49,
    52,
    52,
    50,
    51,
    52,
    52,
    49,
    47,
    48,
    48,
    48,
    50,
    46,
    48,
    47,
    47,
    };
  int nVals = 32;
;

  vtkDoubleArray* dataset1Arr = vtkDoubleArray::New();
  dataset1Arr->SetNumberOfComponents( 1 );
  dataset1Arr->SetName( "Metric 1" );

  vtkDoubleArray* dataset2Arr = vtkDoubleArray::New();
  dataset2Arr->SetNumberOfComponents( 1 );
  dataset2Arr->SetName( "Metric 2" );

  for ( int i = 0; i < nVals; ++ i )
    {
    int ti = i << 1;
    dataset1Arr->InsertNextValue( mingledData[ti] );
    dataset2Arr->InsertNextValue( mingledData[ti + 1] );
    }

  vtkTable* datasetTable = vtkTable::New();
  datasetTable->AddColumn( dataset1Arr );
  dataset1Arr->Delete();
  datasetTable->AddColumn( dataset2Arr );
  dataset2Arr->Delete();

  double nomValue[] = { 49.2188, 49.5 };
  double allowedDev[] = { 1.5 * sqrt( 5.98286 ), 1.5 * sqrt( 7.54839 ) };

  vtkDoubleArray* params1Arr = vtkDoubleArray::New();
  params1Arr->SetNumberOfComponents( 1 );
  params1Arr->SetName( "Params 1" );
  params1Arr->InsertNextValue( nomValue[0] );
  params1Arr->InsertNextValue( allowedDev[0] );

  vtkDoubleArray* params2Arr = vtkDoubleArray::New();
  params2Arr->SetNumberOfComponents( 1 );
  params2Arr->SetName( "Params 2" );
  params2Arr->InsertNextValue( nomValue[1] );
  params2Arr->InsertNextValue( allowedDev[1] );

  vtkTable* paramsTable = vtkTable::New();
  paramsTable->AddColumn( params1Arr );
  params1Arr->Delete();
  paramsTable->AddColumn( params2Arr );
  params2Arr->Delete();

  vtkStatisticsDescriptive* haruspex = vtkStatisticsDescriptive::New();
  haruspex->SetInput( 0, datasetTable );
  haruspex->SetInput( 1, paramsTable );
  vtkTable* outputTable = haruspex->GetOutput();

  datasetTable->Delete();
  paramsTable->Delete();

// -- Test Learn Mode -- 
  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::LearnMode );
  haruspex->Update();
  vtkIdType n = haruspex->GetSampleSize();

  cout << "## Calculated the following statistics ( "
       << n
       << " entries per column ):\n";
  for ( vtkIdType c = 0; c < outputTable->GetNumberOfColumns(); ++ c )
    {
    cout << "   "
         << datasetTable->GetColumnName( c )
         << ": min= "
         << outputTable->GetValue( 0, c ).ToDouble()
         << ", max= "
         << outputTable->GetValue( 1, c ).ToDouble();

    double s[5];
    for ( vtkIdType r = 0; r < 4; ++ r )
      {
      s[r] = outputTable->GetValue( r + 2, c ).ToDouble();
      }
    haruspex->CalculateFromRawMoments( n, s );

    cout << ", mean= "
         << s[0]
         << ", variance= "
         << s[1]
         << ", skewness = "
         << s[2]
         << ", kurtosis excess: sample = "
         << s[3]
         << ", G2 = "
         << s[4]
         << "\n";
    }

// -- Test Evince Mode -- 
  cout << "## Searching for the following outliers:\n";
  for ( vtkIdType c = 0; c < datasetTable->GetNumberOfColumns(); ++ c )
    {
    cout << "   "
         << datasetTable->GetColumnName( c )
         << ": values that deviate of more than "
         << allowedDev[c]
         << " from "
         << nomValue[c]
         << ".\n";
      }

  haruspex->SetExecutionMode( vtkStatisticsAlgorithm::EvinceMode );
  haruspex->Update();

  testIntValue = outputTable->GetNumberOfRows();
  if ( testIntValue != 10 )
    {
    cerr << "Reported an incorrect number of outliers: "
         << testIntValue
         << " != 10.\n";
    return 1;
    }

  cout << "Found "
       << testIntValue
       << " outliers:\n";

  for ( vtkIdType r = 0; r < outputTable->GetNumberOfRows(); ++ r )
    {
    int i = outputTable->GetValue( r, 1 ).ToInt();
    int j = outputTable->GetValue( r, 0 ).ToInt();
    cout << "   "
         << datasetTable->GetColumnName( j )
         << ": "
         << i
         << "-th entry ( "
         << datasetTable->GetValue( i, j ).ToDouble()
         << " ) has a relative deviation of "
         << outputTable->GetValue( r, 2 ).ToDouble()
         << "\n";
    }

  haruspex->Delete();

  return 0;
}

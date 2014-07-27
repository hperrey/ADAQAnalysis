#include <TGFileDialog.h>

#include <sstream>

#include "AASpectrumSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"

AASpectrumSlots::AASpectrumSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
}

AASpectrumSlots::~AASpectrumSlots()
{;}


void AASpectrumSlots::HandleCheckButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();
  
  switch(CheckButtonID){

  case SpectrumCalibration_CB_ID:{
    
    if(TheInterface->SpectrumCalibration_CB->IsDown()){
      TheInterface->SetCalibrationWidgetState(true, kButtonUp);
      TheInterface->HandleTripleSliderPointer();
    }
    else{
      TheInterface->SetCalibrationWidgetState(false, kButtonDisabled);
    }
    break;
  }
  }
}


void AASpectrumSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TheInterface->SaveSettings();

  switch(ComboBoxID){
        // The user can obtain the values used for each calibration point
    // by selecting the calibration point with the combo box
  case SpectrumCalibrationPoint_CBL_ID:{

    const int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();

    // Get the vector<TGraph *> of spectra calibrations from the
    // ComputationMgr (if they are set for this channel) and set the
    // {Energy,PulseUnit} value to the number entry displays
    if(ComputationMgr->GetUseSpectraCalibrations()[Channel]){
      vector<TGraph *> Calibrations = ComputationMgr->GetSpectraCalibrations();

      double *PulseUnit = Calibrations[Channel]->GetX();
      double *Energy = Calibrations[Channel]->GetY();

      // User selected present point ready for entry (not yet set)
      if(SelectedID+1 == TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->GetNumberOfEntries()){
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
	TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
      }
      
      // User selected calibration point (already set)
      else{
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(Energy[SelectedID]);
	TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(PulseUnit[SelectedID]);

	// Update the position of the calibration line
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->ValueSet(0);
      }
    }
    break;
  }

  default:
    break;
  }
}


void AASpectrumSlots::HandleNumberEntries()
{;}


void AASpectrumSlots::HandleRadioButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){

  case ACROSpectrumLS_RB_ID:
    if(TheInterface->ACROSpectrumLS_RB->IsDown()){
      int Entries = ComputationMgr->GetACRONYMLSEntries();
      TheInterface->WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1, Entries);
      TheInterface->WaveformsToHistogram_NEL->GetEntry()->SetNumber(Entries);
    }
    break;

  case ACROSpectrumES_RB_ID:
    if(TheInterface->ACROSpectrumES_RB->IsDown()){
      int Entries = ComputationMgr->GetACRONYMESEntries();
      TheInterface->WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1, Entries);
      TheInterface->WaveformsToHistogram_NEL->GetEntry()->SetNumber(Entries);
    }
    break;
    
  case SpectrumCalibrationStandard_RB_ID:{
    if(TheInterface->SpectrumCalibrationStandard_RB->IsDown()){
      TheInterface->SpectrumCalibrationEdgeFinder_RB->SetState(kButtonUp);
      TheInterface->HandleTripleSliderPointer();
    }
    break;
  }    
    
  case SpectrumCalibrationEdgeFinder_RB_ID:{
    if(TheInterface->SpectrumCalibrationEdgeFinder_RB->IsDown()){
      GraphicsMgr->PlotSpectrum();
      TheInterface->SpectrumCalibrationStandard_RB->SetState(kButtonUp);
    }
    break;
  }
  }
}


void AASpectrumSlots::HandleTextButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGTextButton *TextButton = (TGTextButton *) gTQSender;
  int TextButtonID  = TextButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(TextButtonID){

    ///////////////////
    // Spectra creation
    
  case CreateSpectrum_TB_ID:{
    
    // Alert the user the filtering particles by PSD into the spectra
    // requires integration type peak finder to be used
    int PSDChannel = TheInterface->ADAQSettings->PSDChannel;
    if(ComputationMgr->GetUsePSDFilters()[PSDChannel] and 
       TheInterface->ADAQSettings->ADAQSpectrumIntTypeWW)
      TheInterface->CreateMessageBox("Warning! Use of the PSD filter with spectra creation requires peak finding integration","Asterisk");
    
    // Sequential processing
    if(TheInterface->ProcessingSeq_RB->IsDown()){
      if(TheInterface->ADAQFileLoaded)
	ComputationMgr->CreateSpectrum();

      else if(TheInterface->ACRONYMFileLoaded)
	ComputationMgr->CreateACRONYMSpectrum();
      
      if(ComputationMgr->GetSpectrumExists())
	GraphicsMgr->PlotSpectrum();
    }
    
    // Parallel processing
    else{
      if(TheInterface->ADAQFileLoaded){
	TheInterface->SaveSettings(true);
	ComputationMgr->ProcessWaveformsInParallel("histogramming");
	
	if(ComputationMgr->GetSpectrumExists())
	  GraphicsMgr->PlotSpectrum();
      }
      else if(TheInterface->ACRONYMFileLoaded){
	TheInterface->CreateMessageBox("Error! ACRONYM files cannot be processed in parallel! Please switch to 'sequential mode'.\n","Stop");
      }
    }

    // If the background and integration functions were being used,
    // cycle them in order to refresh the spectral plotting and analysis
    if(TheInterface->SpectrumFindBackground_CB->IsDown()){
      TheInterface->SpectrumFindBackground_CB->SetState(kButtonUp, true);
      TheInterface->SpectrumFindBackground_CB->SetState(kButtonDown, true);
    }

    if(TheInterface->SpectrumFindIntegral_CB->IsDown()){
      TheInterface->SpectrumFindIntegral_CB->SetState(kButtonUp, true);
      TheInterface->SpectrumFindIntegral_CB->SetState(kButtonDown, true);
    }
    
    if(TheInterface->IntegratePearson_CB->IsDown()){
      double DeuteronsInTotal = ComputationMgr->GetDeuteronsInTotal();
      TheInterface->DeuteronsInTotal_NEFL->GetEntry()->SetNumber(DeuteronsInTotal);
    }
    
    int SpectrumMinBin = TheInterface->SpectrumMinBin_NEL->GetEntry()->GetNumber();
    TheInterface->SpectrumRangeMin_NEL->GetEntry()->SetNumber(SpectrumMinBin);
    
    int SpectrumMaxBin = TheInterface->SpectrumMaxBin_NEL->GetEntry()->GetNumber();
    TheInterface->SpectrumRangeMax_NEL->GetEntry()->SetNumber(SpectrumMaxBin);
    break;
  }

    //////////////////////
    // Spectra calibration

  case SpectrumCalibrationSetPoint_TB_ID:{

    // Get the calibration point to be set
    uint SetPoint = TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->GetSelected();

    // Get the energy of the present calibration point
    double Energy = TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->GetNumber();
   
    // Get the pulse unit value of the present calibration point
    double PulseUnit = TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->GetNumber();
    
    // Get the current channel being histogrammed in DGScope
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();

    bool NewPoint = ComputationMgr->SetCalibrationPoint(Channel, SetPoint, Energy, PulseUnit);
    
    if(NewPoint){
      // Add a new point to the number of calibration points in case
      // the user wants to add subsequent points to the calibration
      stringstream ss;
      ss << (SetPoint+1);
      string NewPointLabel = "Calibration point " + ss.str();
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry(NewPointLabel.c_str(),SetPoint+1);
      
      // Set the combo box to display the new calibration point...
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->Select(SetPoint+1);
      
      // ...and set the calibration energy and pulse unit ROOT number
      // entry widgets to their default "0.0" and "1.0" respectively,
      TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
      TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    }
    break;
  }

    ////////////////////
    // Spectra calibrate

  case SpectrumCalibrationCalibrate_TB_ID:{
    
    // If there are 2 or more points in the current channel's
    // calibration data set then create a new TGraph object. The
    // TGraph object will have pulse units [ADC] on the X-axis and the
    // corresponding energies [in whatever units the user has entered
    // the energy] on the Y-axis. A TGraph is used because it provides
    // very easy but extremely powerful methods for interpolation,
    // which allows the pulse height/area to be converted in to energy
    // efficiently in the acquisition loop.

    // Get the current channel being histogrammed in 
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();

    bool Success = ComputationMgr->SetCalibration(Channel);

    if(Success){
      TheInterface->SpectrumCalibrationCalibrate_TB->SetText("Calibrated");
      TheInterface->SpectrumCalibrationCalibrate_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(0));
      TheInterface->SpectrumCalibrationCalibrate_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(32));
    }
    else
      TheInterface->CreateMessageBox("The calibration could not be set!","Stop");

    break;
  }

    ////////////////////////////
    // Spectrum calibration plot

  case SpectrumCalibrationPlot_TB_ID:{
    
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
    GraphicsMgr->PlotCalibration(Channel);

    break;
  }

    /////////////////////////////
    // Spectrum calibration reset

  case SpectrumCalibrationReset_TB_ID:{
    
    // Get the current channel being histogrammed in 
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    bool Success = ComputationMgr->ClearCalibration(Channel);

    // Reset the calibration widgets
    if(Success){
      TheInterface->SpectrumCalibrationCalibrate_TB->SetText("Calibrate");
      TheInterface->SpectrumCalibrationCalibrate_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(1));
      TheInterface->SpectrumCalibrationCalibrate_TB->SetBackgroundColor(TheInterface->ThemeForegroundColor);

      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->RemoveAll();
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry("Calibration point 0", 0);
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->Select(0);
      TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
      TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    }
    break;
  }

  case SpectrumCalibrationLoad_TB_ID:{

    const char *FileTypes[] = {"ADAQ calibration file", "*.acal",
			       "All files",             "*.*",
			       0, 0};
    
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(getenv("PWD"));

    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDOpen, &FileInformation);

    if(FileInformation.fFilename == NULL)
      TheInterface->CreateMessageBox("A calibration file was not selected! No calibration has been made!","Stop");
    else{

      // Get the present channel
      int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
      
      //      string CalibrationFileName = "/home/hartwig/aims/ADAQAnalysis/test/ExptNaIWaveforms.acal";
      string CalibrationFileName = FileInformation.fFilename;

      // Set the calibration file to an input stream
      ifstream In(CalibrationFileName.c_str());

      // Reset an preexisting calibration to make way for the new!
      TheInterface->SpectrumCalibrationReset_TB->Clicked();

      // Iterate through each line in the file and use the data to set
      // the calibration points sequentially
      int SetPoint = 0;
      while(In.good()){
	double Energy, PulseUnit;
	In >> Energy >> PulseUnit;

	if(In.eof()) break;

	ComputationMgr->SetCalibrationPoint(Channel, SetPoint, Energy, PulseUnit);
	
	// Add a new point to the number of calibration points in case
	// the user wants to add subsequent points to the calibration
	stringstream ss;
	ss << (SetPoint+1);
	string NewPointLabel = "Calibration point " + ss.str();
	TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry(NewPointLabel.c_str(),SetPoint+1);
	
	// Set the combo box to display the next setable calibration point...
	TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->Select(SetPoint+1);
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
	TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
	
	SetPoint++;
      }

      In.close();

      // Use the loaded calibration points to set the calibration
      bool Success = ComputationMgr->SetCalibration(Channel);
      if(Success){
	TheInterface->SpectrumCalibrationCalibrate_TB->SetText("Calibrated");
	TheInterface->SpectrumCalibrationCalibrate_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(0));
	TheInterface->SpectrumCalibrationCalibrate_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(32));
      }
      else
	TheInterface->CreateMessageBox("The calibration could not be set! Please check file format.","Stop");
    }
    break;
  }
  }
}
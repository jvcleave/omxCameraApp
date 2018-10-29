#include "StillCameraEngine.h"


StillCameraEngine::StillCameraEngine()
{
	didOpen		= false;
    doWriteFile = false;
    writeFileOnNextPass = false;
    doFillBuffer = false;
    bufferAvailable = false;
    render = NULL;
    camera = NULL;
    encoder = NULL;
    encoderOutputBuffer = NULL;
    hasCreatedRenderTunnel = false;
    displayManager = NULL;
}   


OMX_ERRORTYPE StillCameraEngine::encoderFillBufferDone(OMX_HANDLETYPE hComponent,
                                                       OMX_PTR pAppData,
                                                       OMX_BUFFERHEADERTYPE* pBuffer)
{	
    //ofLogVerbose(__func__);
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    StillCameraEngine *grabber = static_cast<StillCameraEngine*>(pAppData);
    grabber->lock();
    
    grabber->bufferAvailable = true;
    grabber->unlock();
    return error;
}


void StillCameraEngine::setup(OMXCameraSettings& omxCameraSettings_)
{
    settings = omxCameraSettings_;
    OMX_ERRORTYPE error = OMX_ErrorNone;
    
    OMX_CALLBACKTYPE cameraCallbacks;
    
    cameraCallbacks.EventHandler    = &StillCameraEngine::cameraEventHandlerCallback;
    cameraCallbacks.EmptyBufferDone	= &StillCameraEngine::nullEmptyBufferDone;
    cameraCallbacks.FillBufferDone	= &StillCameraEngine::nullFillBufferDone;
    
    error = OMX_GetHandle(&camera, OMX_CAMERA, this , &cameraCallbacks);
    if(error != OMX_ErrorNone) 
    {
        OMX_TRACE(error, "camera OMX_GetHandle FAIL");
    }
    
    //configureCameraResolution();
    error = DisableAllPortsForComponent(&camera);
    OMX_TRACE(error);
    
    
    OMX_CONFIG_REQUESTCALLBACKTYPE cameraCallback;
    OMX_INIT_STRUCTURE(cameraCallback);
    cameraCallback.nPortIndex    =    OMX_ALL;
    cameraCallback.nIndex        =    OMX_IndexParamCameraDeviceNumber;
    cameraCallback.bEnable        =    OMX_TRUE;
    
    error = OMX_SetConfig(camera, OMX_IndexConfigRequestCallback, &cameraCallback);
    OMX_TRACE(error);
    
    //Set the camera (always 0)
    OMX_PARAM_U32TYPE device;
    OMX_INIT_STRUCTURE(device);
    device.nPortIndex    = OMX_ALL;
    device.nU32            = 0;
    
    error = OMX_SetParameter(camera, OMX_IndexParamCameraDeviceNumber, &device);
    OMX_TRACE(error);
    
    //Set the resolution
    OMX_PARAM_PORTDEFINITIONTYPE stillPortConfig;
    OMX_INIT_STRUCTURE(stillPortConfig);
    stillPortConfig.nPortIndex = CAMERA_STILL_OUTPUT_PORT;
    
    error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &stillPortConfig);
    OMX_TRACE(error, "stillPortConfig");
    if(error == OMX_ErrorNone) 
    {
        //ofLogVerbose() << "BEFORE SET";
        //ofLogVerbose() << "sessionConfig.width: " << sessionConfig.width;
        // ofLogVerbose() << "sessionConfig.height: " << sessionConfig.height;
        
       // PrintPortDef(stillPortConfig);
        stillPortConfig.format.image.nFrameWidth        = settings.width;
        stillPortConfig.format.image.nFrameHeight       = settings.height;
        
        stillPortConfig.format.video.nStride            = settings.width;
        
        
        //not setting also works
        //stillPortConfig.format.video.nSliceHeight    = round(settings.height / 16) * 16;
        
        error =  OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &stillPortConfig);
        OMX_TRACE(error);
        error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &stillPortConfig);
       // ofLogVerbose() << "AFTER SET";
        //PrintPortDef(stillPortConfig);
    }
    
    //Set the preview resolution
    OMX_PARAM_PORTDEFINITIONTYPE previewPortConfig;
    OMX_INIT_STRUCTURE(previewPortConfig);
    previewPortConfig.nPortIndex = CAMERA_PREVIEW_PORT;
    
    error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &previewPortConfig);
    OMX_TRACE(error, "previewPortConfig");
    if(error == OMX_ErrorNone) 
    {
        //ofLogVerbose() << "BEFORE SET";
        //ofLogVerbose() << "sessionConfig.width: " << sessionConfig.width;
        // ofLogVerbose() << "sessionConfig.height: " << sessionConfig.height;
        
        //PrintPortDef(previewPortConfig);
        previewPortConfig.format.video.nFrameWidth        = settings.stillPreviewWidth;
        previewPortConfig.format.video.nFrameHeight       = settings.stillPreviewHeight;
        
        previewPortConfig.format.video.nStride            = settings.stillPreviewWidth;
        
        
        //not setting also works
        //previewPortConfig.format.video.nSliceHeight    = round(settings.stillPreviewHeight / 16) * 16;
        
        error =  OMX_SetParameter(camera, OMX_IndexParamPortDefinition, &previewPortConfig);
        OMX_TRACE(error);
        //error =  OMX_GetParameter(camera, OMX_IndexParamPortDefinition, &previewPortConfig);
        //ofLogVerbose() << "AFTER SET";
        //PrintPortDef(previewPortConfig);
    }
}

OMX_ERRORTYPE StillCameraEngine::encoderEventHandlerCallback(OMX_HANDLETYPE hComponent,
                                                            OMX_PTR pAppData,
                                                            OMX_EVENTTYPE eEvent,
                                                            OMX_U32 nData1,
                                                            OMX_U32 nData2,
                                                            OMX_PTR pEventData)
{
//    ofLog(OF_LOG_VERBOSE, 
//          "TextureEngine::%s - eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
//          __func__, eEvent, nData1, nData2, pEventData);
    StillCameraEngine *grabber = static_cast<StillCameraEngine*>(pAppData);

    /*stringstream ss;
    ss << OMX_Maps::getInstance().getEvent(eEvent);
    ofLogVerbose(__func__) << ss.str();*/
    if (eEvent == OMX_EventError)
    {
         OMX_TRACE((OMX_ERRORTYPE)nData1);
    }
    return OMX_ErrorNone;
}


OMX_ERRORTYPE StillCameraEngine::cameraEventHandlerCallback(OMX_HANDLETYPE hComponent,
                                                     OMX_PTR pAppData,
                                                     OMX_EVENTTYPE eEvent,
                                                     OMX_U32 nData1,
                                                     OMX_U32 nData2,
                                                     OMX_PTR pEventData)
{
    /*ofLog(OF_LOG_VERBOSE, 
     "TextureEngine::%s - eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
     __func__, eEvent, nData1, nData2, pEventData);*/
    StillCameraEngine *grabber = static_cast<StillCameraEngine*>(pAppData);
    //ofLogVerbose(__func__) << OMX_Maps::getInstance().getEvent(eEvent);
    switch (eEvent) 
    {
        case OMX_EventParamOrConfigChanged:
        {
            
            return grabber->onCameraEventParamOrConfigChanged();
        }	
            
        case OMX_EventError:
        {
            OMX_TRACE((OMX_ERRORTYPE)nData1);
        }
        default: 
        {
            /*ofLog(OF_LOG_VERBOSE, 
             "TextureEngine::%s - eEvent(0x%x), nData1(0x%lx), nData2(0x%lx), pEventData(0x%p)\n",
             __func__, eEvent, nData1, nData2, pEventData);*/
            
            break;
        }
    }
    return OMX_ErrorNone;
}





int logCounter = 0;


void StillCameraEngine::threadedFunction()
{
	while (isThreadRunning()) 
	{
        if(bufferAvailable)
        {
            
            
            recordingFileBuffer.append((const char*) encoderOutputBuffer->pBuffer + encoderOutputBuffer->nOffset, 
                                       encoderOutputBuffer->nFilledLen);
            
            
            //ofLogVerbose(__func__) << recordingFileBuffer.size();
            bool endOfFrame = (encoderOutputBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME);
            bool endOfStream = (encoderOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS);

            //ofLogVerbose(__func__) << "OMX_BUFFERFLAG_ENDOFFRAME: " << endOfFrame;
            //ofLogVerbose(__func__) << "OMX_BUFFERFLAG_EOS: " << endOfStream;
            
            if(endOfStream || endOfFrame)
            {
                doFillBuffer = false;
                //encoderOutputBuffer->nFilledLen = 0;
                //encoderOutputBuffer->nFlags = 0;
                writeFile();
            }else
            {
                doFillBuffer = true;
            }   
        }
        if(doFillBuffer && encoder)
        {
            doFillBuffer	= false;
            bufferAvailable = false;
            OMX_ERRORTYPE error = OMX_FillThisBuffer(encoder, encoderOutputBuffer);
            OMX_TRACE(error);
        }
    }
}


OMX_ERRORTYPE StillCameraEngine::onCameraEventParamOrConfigChanged()
{
    

    OMX_ERRORTYPE error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error, "camera->OMX_StateIdle");

    //PrintSensorModes(camera);
    
    OMX_FRAMESIZETYPE frameSizeConfig;
    OMX_INIT_STRUCTURE(frameSizeConfig);
    frameSizeConfig.nPortIndex = OMX_ALL;
    
    OMX_PARAM_SENSORMODETYPE sensorMode;
    OMX_INIT_STRUCTURE(sensorMode);
    sensorMode.nPortIndex = OMX_ALL;
    sensorMode.sFrameSize = frameSizeConfig;
    error =OMX_GetParameter(camera, OMX_IndexParamCommonSensorMode, &sensorMode);
    
    OMX_TRACE(error);
    
    sensorMode.bOneShot = OMX_FALSE; //seems to determine whether OMX_BUFFERFLAG_EOS is passed
    error =OMX_SetParameter(camera, OMX_IndexParamCommonSensorMode, &sensorMode);

    OMX_TRACE(error);
    
    OMX_CONFIG_CAMERASENSORMODETYPE sensorConfig;
    OMX_INIT_STRUCTURE(sensorConfig);
    sensorConfig.nPortIndex = OMX_ALL;
    //sensorConfig.nModeIndex = 0;
    error =  OMX_GetParameter(camera, OMX_IndexConfigCameraSensorModes, &sensorConfig);
    OMX_TRACE(error);

    
    OMX_CONFIG_PORTBOOLEANTYPE cameraStillOutputPortConfig;
    OMX_INIT_STRUCTURE(cameraStillOutputPortConfig);
    cameraStillOutputPortConfig.nPortIndex = CAMERA_STILL_OUTPUT_PORT;
    cameraStillOutputPortConfig.bEnabled = OMX_TRUE;
    
    error =OMX_SetParameter(camera, OMX_IndexConfigPortCapturing, &cameraStillOutputPortConfig);	
    OMX_TRACE(error);
    
    
    //Set up renderer
    OMX_CALLBACKTYPE renderCallbacks;
    renderCallbacks.EventHandler	= &StillCameraEngine::nullEventHandlerCallback;
    renderCallbacks.EmptyBufferDone	= &StillCameraEngine::nullEmptyBufferDone;
    renderCallbacks.FillBufferDone	= &StillCameraEngine::nullFillBufferDone;
    

    OMX_GetHandle(&render, OMX_VIDEO_RENDER, this , &renderCallbacks);
    DisableAllPortsForComponent(&render);
    
    
    
    //Set renderer to Idle
    error = OMX_SendCommand(render, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error);
   
    error = buildNonCapturePipeline();
        

    didOpen = true;
    return error;
}

OMX_ERRORTYPE StillCameraEngine::buildNonCapturePipeline()
{
    OMX_ERRORTYPE error;
    
    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error);
    
    error = OMX_SendCommand(render, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error);
    
    if(!hasCreatedRenderTunnel)
    {
        //Create camera->render Tunnel
        error = OMX_SetupTunnel(camera, CAMERA_PREVIEW_PORT, render, VIDEO_RENDER_INPUT_PORT);
        OMX_TRACE(error);
        hasCreatedRenderTunnel = true;
    }
    
    
    //Enable camera preview port
    error = OMX_SendCommand(camera, OMX_CommandPortEnable, CAMERA_PREVIEW_PORT, NULL);
    OMX_TRACE(error);
    
    
    //Enable render input port
    error = OMX_SendCommand(render, OMX_CommandPortEnable, VIDEO_RENDER_INPUT_PORT, NULL);
    OMX_TRACE(error);
    
    
    //Start renderer
    error = OMX_SendCommand(render, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_TRACE(error);
    
    
    displayManager = new DirectDisplay();
    error = displayManager->setup(render, 0, 0, settings.stillPreviewWidth, settings.stillPreviewHeight);

    OMX_TRACE(error);
    
    //Start camera
    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_TRACE(error);
    return error;

}


DirectDisplay* StillCameraEngine::getDisplayManager()
{
    
    return displayManager;
}



bool StillCameraEngine::takePhoto()
{
    bool result = false;
    ofLogVerbose(__func__);
    OMX_ERRORTYPE error;
    
    error = configureEncoder();
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
    error = OMX_SendCommand(encoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
    //Create camera->encoder Tunnel
    error = OMX_SetupTunnel(camera, CAMERA_STILL_OUTPUT_PORT, encoder, IMAGE_ENCODER_INPUT_PORT);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
    //Enable camera output port
    error = OMX_SendCommand(camera, OMX_CommandPortEnable, CAMERA_STILL_OUTPUT_PORT, NULL);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
    //Enable encoder input port
    error = OMX_SendCommand(encoder, OMX_CommandPortEnable, IMAGE_ENCODER_INPUT_PORT, NULL);
    OMX_TRACE(error); 
    if(error != OMX_ErrorNone) return false;
    
    //Enable encoder output port
    error = OMX_SendCommand(encoder, OMX_CommandPortEnable, IMAGE_ENCODER_OUTPUT_PORT, NULL);
    OMX_TRACE(error); 
    if(error != OMX_ErrorNone) return false;
    
    
    error =  OMX_AllocateBuffer(encoder, &encoderOutputBuffer, IMAGE_ENCODER_OUTPUT_PORT, NULL, encoderBufferSize);
    OMX_TRACE(error, "encode OMX_AllocateBuffer");
    if(error != OMX_ErrorNone) return false;
    
    //Start camera
    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
    // Start encoder
    error = OMX_SendCommand(encoder, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
        
    bufferAvailable = false; 
    doFillBuffer = false;    
    bool doThreadBlocking	= true;
    startThread(doThreadBlocking);
    error = OMX_FillThisBuffer(encoder, encoderOutputBuffer);
    if (error != OMX_ErrorNone) 
    {
        ofLogError() << "TAKE PHOTO FAILED";
    }else
    {
        OMX_TRACE(error, "OMX_FillThisBuffer");
        result = true;

    }
    return result;
}
OMX_ERRORTYPE StillCameraEngine::destroyEncoder()
{
    OMX_ERRORTYPE error;
    

    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error);
    
    //Disable camera output port
    error = OMX_SendCommand(camera, OMX_CommandPortDisable, CAMERA_STILL_OUTPUT_PORT, NULL);
    OMX_TRACE(error);
    
    //Disable encoder input port
    error = OMX_SendCommand(encoder, OMX_CommandPortDisable, IMAGE_ENCODER_INPUT_PORT, NULL);
    OMX_TRACE(error);
    
    error = OMX_SetupTunnel(encoder, IMAGE_ENCODER_INPUT_PORT, NULL, 0);
    OMX_TRACE(error);
    
    error =  OMX_SendCommand(encoder, OMX_CommandFlush, IMAGE_ENCODER_INPUT_PORT, NULL);
    OMX_TRACE(error, "encoder: OMX_CommandFlush IMAGE_ENCODER_INPUT_PORT");
    error =  OMX_SendCommand(encoder, OMX_CommandFlush, IMAGE_ENCODER_OUTPUT_PORT, NULL);
    OMX_TRACE(error, "encoder: OMX_CommandFlush IMAGE_ENCODER_OUTPUT_PORT");
    error = DisableAllPortsForComponent(&encoder, "encoder");
    OMX_TRACE(error, "DisableAllPortsForComponent encoder");
    
  
    
    error = OMX_FreeBuffer(encoder, IMAGE_ENCODER_OUTPUT_PORT, encoderOutputBuffer);
    OMX_TRACE(error, "encoder->OMX_FreeBuffer");
    encoderOutputBuffer = NULL;
    
    OMX_STATETYPE encoderState;
    error = OMX_GetState(encoder, &encoderState);
    //OMX_TRACE(error, "encoderState: "+ getStateString(encoderState));
    
    //OMX_CALLBACKTYPE nullCallbacks = {0, 0, 0};
    
    //error =OMX_GetHandle(&encoder, OMX_IMAGE_ENCODER, this , &nullCallbacks);
    //OMX_TRACE(error, "encoderCallbacks");
    
    error = OMX_FreeHandle(encoder);
    OMX_TRACE(error, "OMX_FreeHandle(encoder)"); 
    encoder = NULL;
    return error;
    
}
bool StillCameraEngine::writeFile()
{
    
    stopThread();
    
    destroyEncoder();
 
    buildNonCapturePipeline();
    
    bool result = false;
    string filePath = ofToDataPath(ofGetTimestampString()+".jpg", true);
    
    if(recordingFileBuffer.size()>0)
    {
        result = ofBufferToFile(filePath, recordingFileBuffer);
    }
    doWriteFile = false;
    recordingFileBuffer.clear();
    
    if(result)
    {
        if(listener)
        {
            listener->onTakePhotoComplete(filePath);
        }else
        {
            ofLogWarning(__func__) << filePath << " WRITTEN BUT NO LISTENER SET";
        }
    }
    
    return result;
}


OMX_ERRORTYPE StillCameraEngine::configureEncoder()
{
    
    OMX_ERRORTYPE error;
  

    //Create encoder
    
    OMX_CALLBACKTYPE encoderCallbacks;

    encoderCallbacks.EventHandler		= &StillCameraEngine::encoderEventHandlerCallback;
    encoderCallbacks.EmptyBufferDone	= &StillCameraEngine::nullEmptyBufferDone;
    encoderCallbacks.FillBufferDone		= &StillCameraEngine::encoderFillBufferDone;
    
    error =OMX_GetHandle(&encoder, OMX_IMAGE_ENCODER, this , &encoderCallbacks);
    OMX_TRACE(error, "encoderCallbacks");
    
    error = DisableAllPortsForComponent(&encoder);
    OMX_TRACE(error);
    
    // Encoder input port definition is done automatically upon tunneling
    
    // Configure video format emitted by encoder output port

    OMX_PARAM_PORTDEFINITIONTYPE encoderOutputPortDefinition;
    OMX_INIT_STRUCTURE(encoderOutputPortDefinition);
    encoderOutputPortDefinition.nPortIndex = IMAGE_ENCODER_OUTPUT_PORT;
    error =OMX_GetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
    OMX_TRACE(error);
    if (error == OMX_ErrorNone) 
    {
        //ofLogVerbose() << "ENCODER BEFORE SET";

        
        //PrintPortDef(encoderOutputPortDefinition);
        encoderOutputPortDefinition.format.image.nFrameWidth = settings.width;
        encoderOutputPortDefinition.format.image.nFrameHeight= settings.height;
        encoderOutputPortDefinition.format.image.nStride = settings.width; 
        encoderOutputPortDefinition.format.image.eColorFormat = OMX_COLOR_FormatUnused;
        encoderOutputPortDefinition.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
        error =OMX_SetParameter(encoder, OMX_IndexParamPortDefinition, &encoderOutputPortDefinition);
        OMX_TRACE(error);
        //ofLogVerbose() << "ENCODER AFTER SET";
        //PrintPortDef(encoderOutputPortDefinition);
        
        encoderBufferSize = encoderOutputPortDefinition.nBufferSize;

        
    }
    return error;
    
}

StillCameraEngine::~StillCameraEngine()
{
    if(didOpen)
    {
        closeEngine();
    }
}


void StillCameraEngine::closeEngine()
{
    
    if(isThreadRunning())
    {
        stopThread();
    }
    if(displayManager)
    {
        delete displayManager;
        displayManager = NULL;
        
    }
    OMX_ERRORTYPE error;
    error =  OMX_SendCommand(camera, OMX_CommandFlush, CAMERA_STILL_OUTPUT_PORT, NULL);
    OMX_TRACE(error, "camera: OMX_CommandFlush");
    
    error = DisableAllPortsForComponent(&camera, "camera");
    OMX_TRACE(error, "DisableAllPortsForComponent: camera");
    

    //OMX_StateIdle
    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error, "camera->OMX_StateIdle");

    error = OMX_SendCommand(render, OMX_CommandStateSet, OMX_StateIdle, NULL);
    OMX_TRACE(error, "render->OMX_StateIdle");
    
    if (encoder) 
    {
        error =  OMX_SendCommand(encoder, OMX_CommandFlush, IMAGE_ENCODER_INPUT_PORT, NULL);
        OMX_TRACE(error, "encoder: OMX_CommandFlush IMAGE_ENCODER_INPUT_PORT");
        error =  OMX_SendCommand(encoder, OMX_CommandFlush, IMAGE_ENCODER_OUTPUT_PORT, NULL);
        OMX_TRACE(error, "encoder: OMX_CommandFlush IMAGE_ENCODER_OUTPUT_PORT");
        
        error = DisableAllPortsForComponent(&encoder, "encoder");
        OMX_TRACE(error, "DisableAllPortsForComponent encoder");
        
        error = OMX_FreeBuffer(encoder, IMAGE_ENCODER_OUTPUT_PORT, encoderOutputBuffer);
        OMX_TRACE(error, "encoder->OMX_StateIdle");
        error = OMX_SendCommand(encoder, OMX_CommandStateSet, OMX_StateIdle, NULL);
        OMX_TRACE(error, "encoder->OMX_StateIdle");
        
        
        OMX_STATETYPE encoderState;
        error = OMX_GetState(encoder, &encoderState);
        //OMX_TRACE(error, "encoderState: "+ getStateString(encoderState));
    }
   
    
    //OMX_StateLoaded
    error = OMX_SendCommand(camera, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    OMX_TRACE(error, "camera->OMX_StateLoaded");
    
    error = OMX_SendCommand(render, OMX_CommandStateSet, OMX_StateLoaded, NULL);
    OMX_TRACE(error, "render->OMX_StateLoaded");
    
    if (encoder) 
    {
        error = OMX_SendCommand(encoder, OMX_CommandStateSet, OMX_StateLoaded, NULL);
        OMX_TRACE(error, "encoder->OMX_StateLoaded");
    }

    
    //OMX_FreeHandle
    error = OMX_FreeHandle(camera);
    OMX_TRACE(error, "OMX_FreeHandle(camera)");
    
    error =  OMX_FreeHandle(render);
    OMX_TRACE(error, "OMX_FreeHandle(render)");
    if (encoder) 
    {
        error = OMX_FreeHandle(encoder);
        OMX_TRACE(error, "OMX_FreeHandle(encoder)"); 
    }
    //sessionConfig = NULL;
    didOpen = false;

}

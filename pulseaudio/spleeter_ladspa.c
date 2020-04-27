/* spleeter_ladspa.c
 */

/*****************************************************************************/

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"
#include "request.c"

/*****************************************************************************/

/* The port numbers for the plugin: */

#define SDL_INPUT        0
#define SDL_OUTPUT       1

/*****************************************************************************/

/* Instance data for the simple delay line plugin. */

unsigned long ConntectedChannels = 0;

typedef struct {
  unsigned long m_lChannelNumber;

  unsigned long m_fSampleRate;

  int socket;

  LADSPA_Data * m_pfBuffer;

  LADSPA_Data * m_pfBufferProcessed;
 
  LADSPA_Data * m_pfTempBuffer;
 
  /* Buffer size. */
  unsigned long m_lBufferSize;

  /* Write pointer in buffer. */
  unsigned long m_lWritePointer;

  /* Write pointer in buffer. */
  unsigned long m_lLastProcessedPointer;

  /* Ports:
     ------ */

  /* Input audio port data location. */
  LADSPA_Data * m_pfInput;

  /* Output audio port data location. */
  LADSPA_Data * m_pfOutput;

} SpleeterAdapter;

/*****************************************************************************/

/* Construct a new plugin instance. */
LADSPA_Handle 
instantiateSpleeterAdapter(const LADSPA_Descriptor * Descriptor,
			   unsigned long             SampleRate) {

  SpleeterAdapter * psSpleeterAdapter;

  psSpleeterAdapter 
    = (SpleeterAdapter *)malloc(sizeof(SpleeterAdapter));

  if (psSpleeterAdapter == NULL) 
    return NULL;

  psSpleeterAdapter->m_lChannelNumber = ConntectedChannels;
  ConntectedChannels++;
  
  psSpleeterAdapter->m_fSampleRate = SampleRate;
  psSpleeterAdapter->m_lBufferSize = 5 * SampleRate;

  psSpleeterAdapter->m_pfBuffer 
    = (LADSPA_Data *)calloc(psSpleeterAdapter->m_lBufferSize, sizeof(LADSPA_Data));

  psSpleeterAdapter->m_pfBufferProcessed 
    = (LADSPA_Data *)calloc(psSpleeterAdapter->m_lBufferSize, sizeof(LADSPA_Data));

  psSpleeterAdapter->m_pfTempBuffer 
    = (LADSPA_Data *)calloc(psSpleeterAdapter->m_lBufferSize, sizeof(LADSPA_Data));

  psSpleeterAdapter->m_lLastProcessedPointer = 0; 
  psSpleeterAdapter->m_lWritePointer = 0; 

  psSpleeterAdapter->socket = create_socket();

  return psSpleeterAdapter;
}

/*****************************************************************************/

/* Initialise and activate a plugin instance. */
void
activateSpleeterAdapter(LADSPA_Handle Instance) {

  SpleeterAdapter * psSpleeterAdapter;
  psSpleeterAdapter = (SpleeterAdapter *)Instance;

  // reset connected channels
  ConntectedChannels = 0;

  /* Need to reset the delay history in this function rather than
     instantiate() in case deactivate() followed by activate() have
     been called to reinitialise a delay line. */
  memset(psSpleeterAdapter->m_pfBuffer, 
        0, 
        sizeof(LADSPA_Data) * psSpleeterAdapter->m_lBufferSize);
  memset(psSpleeterAdapter->m_pfBufferProcessed, 
        0, 
        sizeof(LADSPA_Data) * psSpleeterAdapter->m_lBufferSize);
  memset(psSpleeterAdapter->m_pfTempBuffer, 
        0, 
        sizeof(LADSPA_Data) * psSpleeterAdapter->m_lBufferSize);
 }


/* Connect a port to a data location. */
void 
connectPortToSpleeterAdapter(LADSPA_Handle Instance,
			     unsigned long Port,
			     LADSPA_Data * DataLocation) {

  SpleeterAdapter * psSpleeterAdapter;

  psSpleeterAdapter = (SpleeterAdapter *)Instance;
  switch (Port) {
  case SDL_INPUT:
    psSpleeterAdapter->m_pfInput = DataLocation;
    break;
  case SDL_OUTPUT:
    psSpleeterAdapter->m_pfOutput = DataLocation;
    break;
  }
}

unsigned long copy_from(LADSPA_Data *buffer, LADSPA_Data *src, unsigned long buffer_size, unsigned long from, unsigned long len) {
  if (len < buffer_size - from) {
    memcpy(buffer + from, src, len * sizeof(LADSPA_Data));
    return from + len;
  } else {
    memcpy(buffer + from, src, (buffer_size - from) * sizeof(LADSPA_Data));
    memcpy(buffer, src + (buffer_size - from), (len - (buffer_size - from)) * sizeof(LADSPA_Data));
    return len - (buffer_size - from);
  }
}

unsigned long copy_to(LADSPA_Data *buffer, LADSPA_Data *dest, unsigned long buffer_size, unsigned long from, unsigned long len) {
  if (len < buffer_size - from) {
    memcpy(dest, buffer + from, len * sizeof(LADSPA_Data));
    return from + len;
  } else {
    memcpy(dest, buffer + from, (buffer_size - from) * sizeof(LADSPA_Data));
    memcpy(dest + (buffer_size - from), buffer, (len - (buffer_size - from)) * sizeof(LADSPA_Data));
    return len - (buffer_size - from);
  }
}

int is_between(unsigned long val, unsigned long start, unsigned long end) {
  if (start < end) {
    return val >= start && val < end;
  } else {
    return val >= start || val < end;
  }
}

long mod(unsigned long size, long val) {
  while (val < 0) {
    val += size;
  }
  while (val >= size) {
    val -= size;
  }
  return val;
}

/*****************************************************************************/

/* Run a delay line instance for a block of SampleCount samples. */
void 
runSpleeterAdapter(LADSPA_Handle Instance,
		   unsigned long SampleCount) {

  unsigned long diff;
  
  LADSPA_Data * pfInput;
  LADSPA_Data * pfOutput;
  SpleeterAdapter * psSpleeterAdapter;

  unsigned long newWrite;
  long newRead;
  long lastProcessed;


  psSpleeterAdapter = (SpleeterAdapter *)Instance;

  unsigned long size = psSpleeterAdapter->m_lBufferSize;

  pfInput = psSpleeterAdapter->m_pfInput;
  pfOutput = psSpleeterAdapter->m_pfOutput;

  long delay = 4410 * 1;

  newWrite = copy_from(psSpleeterAdapter->m_pfBuffer, pfInput, size, psSpleeterAdapter->m_lWritePointer, SampleCount);

  diff = psSpleeterAdapter->m_fSampleRate * 3 / 2;
  newRead = mod(size, newWrite - diff);

  lastProcessed = mod(size, psSpleeterAdapter->m_lLastProcessedPointer - diff);

  if (!is_between(mod(size, psSpleeterAdapter->m_lWritePointer - delay + SampleCount - 1), lastProcessed, psSpleeterAdapter->m_lLastProcessedPointer)) {
    copy_to(psSpleeterAdapter->m_pfBuffer, psSpleeterAdapter->m_pfTempBuffer, size, newRead, diff);
    send_sound(psSpleeterAdapter->socket, psSpleeterAdapter->m_lChannelNumber, psSpleeterAdapter->m_fSampleRate,
      diff,
      psSpleeterAdapter->m_pfTempBuffer,
      psSpleeterAdapter->m_pfTempBuffer
    );
    psSpleeterAdapter->m_lLastProcessedPointer = copy_from(psSpleeterAdapter->m_pfBufferProcessed, psSpleeterAdapter->m_pfTempBuffer, size, newRead, diff);
  }

  copy_to(psSpleeterAdapter->m_pfBufferProcessed, pfOutput, size, mod(size, psSpleeterAdapter->m_lWritePointer - delay), SampleCount);
  psSpleeterAdapter->m_lWritePointer = newWrite;
}

/*****************************************************************************/

/* Throw away a simple delay line. */
void 
cleanupSpleeterAdapter(LADSPA_Handle Instance) {

  SpleeterAdapter * psSpleeterAdapter;

  psSpleeterAdapter = (SpleeterAdapter *)Instance;

  close(psSpleeterAdapter->socket);
  free(psSpleeterAdapter->m_pfBufferProcessed);
  free(psSpleeterAdapter->m_pfBuffer);
  free(psSpleeterAdapter->m_pfTempBuffer);
  free(psSpleeterAdapter);
}

/*****************************************************************************/

LADSPA_Descriptor * g_psDescriptor = NULL;

/*****************************************************************************/

/* _init() is called automatically when the plugin library is first
   loaded. */
void 
_init() {

  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psDescriptor
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
  if (g_psDescriptor) {
    g_psDescriptor->UniqueID
      = 139439024;
    g_psDescriptor->Label
      = strdup("vocals");
    g_psDescriptor->Properties
      = 0;
    g_psDescriptor->Name 
      = strdup("Vocals");
    g_psDescriptor->Maker
      = strdup("Mohamed Elawadi");
    g_psDescriptor->Copyright
      = strdup("None");
    g_psDescriptor->PortCount 
      = 2;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(2, sizeof(LADSPA_PortDescriptor));
    g_psDescriptor->PortDescriptors 
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[SDL_INPUT]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[SDL_OUTPUT]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    pcPortNames
      = (char **)calloc(2, sizeof(char *));
    g_psDescriptor->PortNames
      = (const char **)pcPortNames;
    pcPortNames[SDL_INPUT] 
      = strdup("Input");
    pcPortNames[SDL_OUTPUT]
      = strdup("Output");
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(2, sizeof(LADSPA_PortRangeHint)));
    g_psDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[SDL_INPUT].HintDescriptor
      = 0;
    psPortRangeHints[SDL_OUTPUT].HintDescriptor
      = 0;
    g_psDescriptor->instantiate
      = instantiateSpleeterAdapter;
    g_psDescriptor->connect_port 
      = connectPortToSpleeterAdapter;
    g_psDescriptor->activate
      = activateSpleeterAdapter;
    g_psDescriptor->run 
      = runSpleeterAdapter;
    g_psDescriptor->run_adding
      = NULL;
    g_psDescriptor->set_run_adding_gain
      = NULL;
    g_psDescriptor->deactivate
      = NULL;
    g_psDescriptor->cleanup
      = cleanupSpleeterAdapter;
  }
}

/*****************************************************************************/

/* _fini() is called automatically when the library is unloaded. */
void 
_fini() {
  long lIndex;
  if (g_psDescriptor) {
    free((char *)g_psDescriptor->Label);
    free((char *)g_psDescriptor->Name);
    free((char *)g_psDescriptor->Maker);
    free((char *)g_psDescriptor->Copyright);
    free((LADSPA_PortDescriptor *)g_psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < g_psDescriptor->PortCount; lIndex++)
      free((char *)(g_psDescriptor->PortNames[lIndex]));
    free((char **)g_psDescriptor->PortNames);
    free((LADSPA_PortRangeHint *)g_psDescriptor->PortRangeHints);
    free(g_psDescriptor);
  }
}

/*****************************************************************************/

/* Return a descriptor of the requested plugin type. Only one plugin
   type is available in this library. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  if (Index == 0)
    return g_psDescriptor;
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */

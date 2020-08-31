/*******************************************************************************
 * Copyright (c) 2016, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <stdlib.h>

#include "cipelectronickey.h"
#include "cipconnectionobject.h"
#include "cipconnectionmanager.h"
#include "cipidentity.h"
#include "trace.h"

void ElectronicKeySetKeyFormat(CipElectronicKey *const electronic_key,
                               const CipUsint key_format) {
  electronic_key->key_format = key_format;
}

CipUint ElectronicKeyGetKeyFormat(const CipElectronicKey *const electronic_key)
{
  return electronic_key->key_format;
}

void ElectronicKeySetKeyData(CipElectronicKey *const electronic_key,
                             void *key_data) {
  electronic_key->key_data = key_data;
}

void *ElectronicKeyGetKeyData(const CipElectronicKey *const electronic_key) {
  return electronic_key->key_data;
}

typedef struct electronic_key_format_4 {
  CipUint vendor_id;
  CipUint device_type;
  CipUint product_code;
  CipByte major_revision_compatibility;
  CipUsint minor_revision;
} ElectronicKeyFormat4;

const size_t kElectronicKeyFormat4Size = sizeof(ElectronicKeyFormat4);

ElectronicKeyFormat4 *ElectronicKeyFormat4New() {
  return (ElectronicKeyFormat4 *)calloc( 1, sizeof(ElectronicKeyFormat4) );
}

void ElectronicKeyFormat4Delete(ElectronicKeyFormat4 **electronic_key) {
  free(*electronic_key);
  *electronic_key = NULL;
}

void ElectronicKeyFormat4SetVendorId(ElectronicKeyFormat4 *const electronic_key,
                                     const CipUint vendor_id) {
  electronic_key->vendor_id = vendor_id;
}

CipUint ElectronicKeyFormat4GetVendorId(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->vendor_id;
}

void ElectronicKeyFormat4SetDeviceType(
  ElectronicKeyFormat4 *const electronic_key,
  const CipUint device_type) {
  electronic_key->device_type = device_type;
}

CipUint ElectronicKeyFormat4GetDeviceType(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->device_type;
}

void ElectronicKeyFormat4SetProductCode(
  ElectronicKeyFormat4 *const electronic_key,
  const CipUint product_code) {
  electronic_key->product_code = product_code;
}

CipUint ElectronicKeyFormat4GetProductCode(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->product_code;
}

void ElectronicKeyFormat4SetMajorRevisionCompatibility(
  ElectronicKeyFormat4 *const electronic_key,
  const CipByte major_revision_compatibility) {
  electronic_key->major_revision_compatibility = major_revision_compatibility;
}

CipByte ElectronicKeyFormat4GetMajorRevision(
  const ElectronicKeyFormat4 *const electronic_key) {
  const CipByte kMajorRevisionMask = 0x7F;
  return (electronic_key->major_revision_compatibility & kMajorRevisionMask);
}

bool ElectronicKeyFormat4GetMajorRevisionCompatibility(
  const ElectronicKeyFormat4 *const electronic_key) {
  const CipByte kCompatibilityMask = 0x80;
  if( kCompatibilityMask ==
      (electronic_key->major_revision_compatibility & kCompatibilityMask) ) {
    return true;
  }
  return false;
}

void ElectronicKeyFormat4SetMinorRevision(
  ElectronicKeyFormat4 *const electronic_key,
  const CipUsint minor_revision) {
  electronic_key->minor_revision = minor_revision;
}

CipUsint ElectronicKeyFormat4GetMinorRevision(
  const ElectronicKeyFormat4 *const electronic_key) {
  return electronic_key->minor_revision;
}

EipStatus CheckElectronicKeyData(
  EipUint8 key_format,
  void *key_data,
  EipUint16 *extended_status
  ) {
  /* Default return value */
  *extended_status = kConnectionManagerExtendedStatusCodeSuccess;

  /* Check key format */
  if (4 != key_format) {
    *extended_status =
      kConnectionManagerExtendedStatusCodeErrorInvalidSegmentTypeInPath;
    return kEipStatusError;
  }

  bool compatiblity_mode = ElectronicKeyFormat4GetMajorRevisionCompatibility(
    key_data);

  /* Check VendorID and ProductCode, must match, or 0 */
  if ( ( (ElectronicKeyFormat4GetVendorId(key_data) != g_identity.vendor_id) &&
         (ElectronicKeyFormat4GetVendorId(key_data) != 0) )
       || ( (ElectronicKeyFormat4GetProductCode(key_data) !=
             g_identity.product_code)
            && (ElectronicKeyFormat4GetProductCode(key_data) != 0) ) ) {
    *extended_status =
      kConnectionManagerExtendedStatusCodeErrorVendorIdOrProductcodeError;
    return kEipStatusError;
  } else {
    /* VendorID and ProductCode are correct */

    /* Check DeviceType, must match or 0 */
    if ( (ElectronicKeyFormat4GetDeviceType(key_data) != g_identity.device_type)
         && (ElectronicKeyFormat4GetDeviceType(key_data) != 0) ) {
      *extended_status =
        kConnectionManagerExtendedStatusCodeErrorDeviceTypeError;
      return kEipStatusError;
    } else {
      /* VendorID, ProductCode and DeviceType are correct */

      if (false == compatiblity_mode) {
        /* Major = 0 is valid */
        if (0 == ElectronicKeyFormat4GetMajorRevision(key_data) ) {
          return kEipStatusOk;
        }

        /* Check Major / Minor Revision, Major must match, Minor match or 0 */
        if ( (ElectronicKeyFormat4GetMajorRevision(key_data) !=
              g_identity.revision.major_revision)
             || ( (ElectronicKeyFormat4GetMinorRevision(key_data) !=
                   g_identity.revision.minor_revision)
                  && (ElectronicKeyFormat4GetMinorRevision(key_data) != 0) ) ) {
          *extended_status =
            kConnectionManagerExtendedStatusCodeErrorRevisionMismatch;
          return kEipStatusError;
        }
      } else {
        /* Compatibility mode is set */

        /* Major must match, Minor != 0 and <= MinorRevision */
        if ( (ElectronicKeyFormat4GetMajorRevision(key_data) ==
              g_identity.revision.major_revision)
             && (ElectronicKeyFormat4GetMinorRevision(key_data) > 0)
             && (ElectronicKeyFormat4GetMinorRevision(key_data) <=
                 g_identity.revision.minor_revision) ) {
          return kEipStatusOk;
        } else {
          *extended_status =
            kConnectionManagerExtendedStatusCodeErrorRevisionMismatch;
          return kEipStatusError;
        }
      } /* end if CompatiblityMode handling */
    }
  }

  return
    (*extended_status == kConnectionManagerExtendedStatusCodeSuccess) ?
    kEipStatusOk : kEipStatusError;
}

EipStatus CheckElectronicKey(CipConnectionObject *connection_object,
		CipMessageRouterRequest *message_router_request, size_t remaining_path,
		EipUint16 *extended_status) {

	const EipUint8 *message = message_router_request->data;
	if (kElectronicKeySegmentFormatKeyFormat4
			== GetPathLogicalSegmentElectronicKeyFormat(message)) {
		/* Check if there is enough data for holding the electronic key segment */
		if (remaining_path < 5) {
			//*extended_error = 0;
			OPENER_TRACE_INFO("Message not long enough for electronic key\n");
			return kCipErrorNotEnoughData;
		}
		/* Electronic key format 4 found */
		connection_object->electronic_key.key_format = 4;
		ElectronicKeyFormat4 *electronic_key = ElectronicKeyFormat4New();
		GetElectronicKeyFormat4FromMessage(&message, electronic_key);
		/* logical electronic key found */
		connection_object->electronic_key.key_data = electronic_key;

		remaining_path -= 5; /*length of the electronic key*/
		OPENER_TRACE_INFO(
				"key: ven ID %d, dev type %d, prod code %d, major %d, minor %d\n",
				ElectronicKeyFormat4GetVendorId(connection_object->electronic_key.
						key_data),
				ElectronicKeyFormat4GetDeviceType(connection_object->
						electronic_key.key_data),
				ElectronicKeyFormat4GetProductCode(connection_object->
						electronic_key.key_data),
				ElectronicKeyFormat4GetMajorRevision(connection_object->
						electronic_key.key_data),
				ElectronicKeyFormat4GetMinorRevision(connection_object->
						electronic_key.key_data) );
		if (kEipStatusOk
				!= CheckElectronicKeyData(
						connection_object->electronic_key.key_format,
						connection_object->electronic_key.key_data,
						extended_status)) {
			ElectronicKeyFormat4Delete(&electronic_key);
			return kCipErrorConnectionFailure;
		}
		ElectronicKeyFormat4Delete(&electronic_key);
	}
	return kCipErrorSuccess;
}

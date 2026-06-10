#pragma once
#if NV_USE_HLA

#include <nvk_common.h>  // String, Vector<>

namespace nv {

/// Configuration for joining an HLA 1516e federation as a passive subscriber.
/// Plain data struct — no RTI dependency.
struct HlaFederateConfig {

    /// Name of the federation execution to join.
    /// The federation must already exist (created by the sim or another federate).
    String federationName = "MySim";

    /// Name this federate will register under in the federation.
    /// Must be unique within the federation execution.
    String federateName = "NervSDK_3DViewer";

    /// OpenRTI connection string passed to RTIambassador::connect().
    /// Format: "crcAddress=<host>:<port>"  (OpenRTI / HLA Evolved)
    /// For a MÄK RTI server, the CRC typically listens on port 8989:
    ///   "crcAddress=sim-server:8989"
    /// Leave as default to connect to localhost.
    String rtiAddress = "crcAddress=localhost";

    /// Paths to FOM XML modules to pass to createFederationExecution().
    /// At minimum: the RPR-FOM 2.0 base module.
    /// Example: { "RPR_FOM_v2.0_Base_Object_Model.xml" }
    /// Paths may be absolute or relative to the working directory.
    /// Passed verbatim as file:// URIs to the RTI.
    Vector<String> fomPaths;

    /// Logical time implementation name used when creating the federation.
    /// "HLAfloat64Time" is standard and used by RPR-FOM 2.0.
    String logicalTimeImpl = "HLAfloat64Time";
};

} // namespace nv

#endif // NV_USE_HLA

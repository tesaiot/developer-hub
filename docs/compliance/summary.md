# บทวิเคราะห์มาตราฐานความปลอดภัย และ TESA AIoT Foundation Platform

## บทนำและความสำคัญของมาตราฐานความปลอดภัย

ในปัจจุบัน อุปกรณ์ Internet of Things (IoT) มีการใช้งานอย่างแพร่หลาย ทั้งในภาคอุตสาหกรรม (IIoT) และภาคประชาชน (Consumer IoT) การปฏิบัติตามมาตรฐานความปลอดภัย (Compliance) ไม่ใช่เพียงแค่การทำตามกฎระเบียบ แต่เป็น "ใบเบิกทางทางการค้า (Trade Passport)" ที่สำคัญที่สุดในการส่งออกสินค้าเทคโนโลยีของไทยไปสู่ตลาดโลก โดยมีความสำคัญในมิติต่างๆดังนี้

*   **การเข้าถึงตลาดสากล (Market Access):** สหภาพยุโรป สหรัฐอเมริกา และสิงคโปร์ ได้เริ่มบังคับใช้กฎหมายความปลอดภัยทางไซเบอร์สำหรับอุปกรณ์ IoT การขาด Compliance จะเปรียบเสมือนกำแพงภาษีที่มองไม่เห็น (Non-Tariff Barrier) ที่กีดกันสินค้าไทย
*   **ความเชื่อมั่นของผู้บริโภค (Consumer Trust):** ฉลากรับรองความปลอดภัย (Cybersecurity Label) ช่วยสร้างความมั่นใจให้ผู้ใช้งานว่าอุปกรณ์จะไม่ถูกเจาะระบบหรือละเมิดความเป็นส่วนตัว
*   **การลดความเสี่ยงทางกฎหมาย (Liability Reduction):** การปฏิบัติตามมาตรฐานช่วยลดความเสี่ยงจากการถูกฟ้องร้องเมื่อเกิดเหตุการณ์ข้อมูลรั่วไหล ซึ่งสอดคล้องกับ พ.ร.บ. คุ้มครองข้อมูลส่วนบุคคล (PDPA) ของไทยและ GDPR ของยุโรป

## สรุปมาตราฐานความปลอดภัยที่สำคัญระดับโลก

ปัจจุบันมีมาตรฐานหลักที่ได้รับการยอมรับและถูกนำไปใช้เป็นต้นแบบกฎหมายในหลายประเทศ ดังนี้

**1. ETSI EN 303 645 (ยุโรป/สากล)**
*   **สถานะ:** ถือเป็นมาตรฐาน "Golden Standard" ของโลกสำหรับ Consumer IoT
*   **สาระสำคัญ:** กำหนดข้อกำหนดพื้นฐาน 13 ข้อ เช่น ห้ามใช้ Default Password, ต้องมีนโยบายการเปิดเผยช่องโหว่ (Vulnerability Disclosure), และต้องมีการอัปเดตซอฟต์แวร์

**2. NIST IR 8259 Series (สหรัฐอเมริกา)**
*   **สถานะ:** แนวปฏิบัติจากรัฐบาลสหรัฐฯ ซึ่งเป็นรากฐานของฉลาก "U.S. Cyber Trust Mark"
*   **สาระสำคัญ:** เน้นที่ผลลัพธ์ (Outcomes-based) เช่น การระบุตัวตนอุปกรณ์ (Identification), การตั้งค่าความปลอดภัย (Configuration), และการปกป้องข้อมูล (Data Protection)

**3. UK PSTI Act (สหราชอาณาจักร)**
*   **สถานะ:** กฎหมายบังคับใช้ (Mandatory Law)
*   **สาระสำคัญ:** บังคับใช้ 3 ข้อหลักอย่างเคร่งครัด (Password, Reporting, Updates) หากไม่ทำตามจะไม่สามารถวางจำหน่ายสินค้าใน UK ได้

**4. Cybersecurity Labelling Scheme (CLS) (สิงคโปร์)**
### 4.1 CLS IoT (Cybersecurity Labelling Scheme for IoT Devices)
*   **สถานะปัจจุบัน (Current Status):**
    *   **ผู้นำภูมิภาคและสากล:** เป็นโครงการแรกในภูมิภาคเอเชียแปซิฟิกที่เปิดตัวฉลากความปลอดภัยไซเบอร์สำหรับอุปกรณ์ Smart Devices
    *   **การยอมรับร่วม (Mutual Recognition Arrangement - MRA):** นี่คือจุดแข็งที่สุด โดยสิงคโปร์ได้ลงนาม MRA กับประเทศฟินแลนด์ (Traficom) และเยอรมนี (BSI) ส่งผลให้สินค้าที่ผ่าน CLS ระดับที่กำหนด สามารถวางขายในประเทศคู่สัญญาได้ทันทีโดยไม่ต้องทดสอบซ้ำ
    *   **ก้าวสู่กฎหมายบังคับ (Mandatory Roadmap):** ขณะนี้ CSA กำลังอยู่ในระหว่างกระบวนการทางกฎหมายเพื่อ "บังคับ" ให้สินค้าระดับ High-risk (เช่น Wi-Fi Routers และ Smart Home Hubs) ต้อง ได้รับฉลาก CLS ขั้นต่ำ Level 1 จึงจะวางจำหน่ายในสิงคโปร์ได้ (จากเดิมที่เป็นภาคสมัครใจ)
*   **สาระสำคัญเชิงลึก (Key Substance):** ระบบแบ่งออกเป็น 4 ระดับ เพื่อรองรับความหลากหลายของอุปกรณ์ ตั้งแต่เซนเซอร์ราคาถูกไปจนถึงกล้องวงจรปิดความเสี่ยงสูง:
    *   **Level 1 (Basic Requirement):**
        *   **วิธีการ:** ผู้ผลิตรับรองตนเอง (Self-Declaration)
        *   **ข้อกำหนด:** ต้องผ่านเกณฑ์พื้นฐาน 2 ข้อหลักตามมาตรฐาน ETSI EN 303 645 คือ (1) ไม่มีรหัสผ่านตั้งค่าจากโรงงานที่ซ้ำกันทุกเครื่อง (No Universal Default Password) และ (2) มีช่องทางและกระบวนการอัปเดตซอฟต์แวร์ที่ชัดเจน
    *   **Level 2 (Security by Design):**
        *   **วิธีการ:** ตรวจสอบโดยหน่วยงานภายนอก (Third-party Review)
        *   **ข้อกำหนด:** เพิ่มเติมจาก Level 1 ครอบคลุมหลักการ "Security by Design" การประเมินความเสี่ยง และการจัดการข้อมูลส่วนบุคคล
    *   **Level 3 (Absence of Known Threats):**
        *   **วิธีการ:** ทดสอบโดยห้องปฏิบัติการ (Lab Testing)
        *   **ข้อกำหนด:** เน้นการทดสอบซอฟต์แวร์ (Software Binary Analysis) เพื่อยืนยันว่าไม่มีบั๊กหรือช่องโหว่ร้ายแรงที่เปิดเผยต่อสาธารณะ (Known Vulnerabilities)
    *   **Level 4 (Resistance to Attacks):**
        *   **วิธีการ:** ทดสอบเจาะระบบขั้นสูง (Penetration Testing)
        *   **ข้อกำหนด:** เป็นระดับสูงสุด ต้องทนทานต่อการโจมตีทางไซเบอร์ในรูปแบบต่างๆ ได้ เหมาะสำหรับอุปกรณ์ที่มีความสำคัญสูง

### 4.2 CLS Ready (Cybersecurity Labelling Scheme for Ready)
*   **สถานะปัจจุบัน (Current Status):**
    *   **นวัตกรรมแก้ปัญหาคอขวด (Adoption Accelerator):** CSA เปิดตัว CLS Ready เพื่อแก้ปัญหา "ค่าใช้จ่ายสูง" และ "ใช้เวลานาน" ในการขอรับรองสินค้า IoT โดยเฉพาะสำหรับผู้ประกอบการรายย่อย (SMEs) ที่มักนำชิ้นส่วนสำเร็จรูปมาประกอบ
    *   **การขยายตัวของ Ecosystem:** ปัจจุบันบริษัทผู้ผลิตชิปและแพลตฟอร์มระดับโลก (เช่น ผู้ผลิตชิป Wi-Fi หรือ Cloud Providers) เริ่มนำผลิตภัณฑ์ของตนมาขอขึ้นทะเบียน CLS Ready เพื่อดึงดูดให้ผู้ผลิตอุปกรณ์ปลายทางเลือกใช้สินค้าของตน
    *   **เป็นมาตราฐานเพื่อเตรียมความพร้อมเพื่อให้ได้รับ CLS IoT level 4**
*   **สาระสำคัญเชิงลึก (Key Substance):** CLS Ready ไม่ใช่ฉลากสำหรับ "สินค้าที่ขายให้ผู้บริโภค" แต่เป็นฉลากสำหรับ "ส่วนประกอบ (Components)"
    *   **หลักการสืบทอดคุณสมบัติ (Inheritance Mechanism):**
        *   หากผู้ผลิตอุปกรณ์ IoT (End Product) เลือกใช้โมดูล Wi-Fi หรือระบบปฏิบัติการ (OS) ที่ได้ตรา CLS Ready
        *   ผู้ผลิตสามารถนำผลการรับรองนั้นมา "เคลมคะแนน" ในส่วนที่เกี่ยวข้องได้ทันที โดยไม่ต้องส่งทดสอบหัวข้อนั้นซ้ำ
    *   **ครอบคลุม 4 หมวดหลัก:**
        *   System-on-Chip (SoC): ชิปประมวลผล
        *   Connectivity Modules: โมดูลสื่อสาร เช่น Bluetooth, Wi-Fi
        *   Operating Systems / Software: ซอฟต์แวร์ระบบ
        *   Cloud Services: บริการคลาวด์ที่อุปกรณ์เชื่อมต่อ
    *   **ผลลัพธ์:** ช่วยให้ผู้ผลิตสินค้า IoT (โดยเฉพาะสินค้าไทยที่เป็น System Integrator) สามารถขอรับฉลาก CLS Level 1 ได้ง่ายขึ้น เร็วขึ้น และประหยัดต้นทุนการทดสอบลงอย่างมาก

**5 NCSA IoT Cybersecurity Guideline (ไทย)**
*   **สถานะ:** อยู่ระหว่างการทำประชามติเพื่อเป็นราชกิจจานุเบกษาประกาศกฎหมายและมาตรฐานภายในเดือนเมษายน 2569
*   **สาระสำคัญ:** ยึดหลักการบริหารจัดการความเสี่ยง (Risk-Based Approach) และการรักษาความปลอดภัยตลอดวงจรชีวิต (Lifecycle Security) ตั้งแต่การออกแบบไปจนถึงการเลิกใช้งาน ข้อกำหนดแบ่งออกเป็น 4 ระดับ โดย
    *   **ระดับที่ 1** เป็นเกณฑ์พื้นฐานที่ต้องไม่มีการใช้รหัสผ่านเริ่มต้นซ้ำกัน (No Default Passwords) และต้องมีช่องทางรับแจ้งช่องโหว่จากสาธารณะ
    *   **ระดับที่ 2** จะต้องมีการเข้ารหัสข้อมูลสำคัญทั้งที่จัดเก็บและรับส่งผ่านเครือข่าย รวมถึงปิดฟังก์ชันหรือพอร์ตการเชื่อมต่อที่ไม่จำเป็นเพื่อลดโอกาสถูกโจมตี
    *   **ระดับที่ 3** กำหนดให้ผู้ผลิตต้องมีการทำแบบจำลองภัยคุกคาม (Threat Modelling) และจัดทำบัญชีรายการส่วนประกอบซอฟต์แวร์ (SBOM) เพื่อบริหารจัดการช่องโหว่
    *   **ระดับที่ 4** ต้องมีการทดสอบเจาะระบบแบบกล่องดำ (Black Box Penetration Testing) เพื่อค้นหาจุดอ่อนที่อาจถูกผู้ไม่หวังดีโจมตี

## CLS Ready สำหรับ TESA AIoT Foundation Platform

TESA AIoT Foundation Platform สร้างบนพื้นฐานของ CLS Ready จาก Infineon Technologies OPTIGA Trust M และ PSoC ทำให้อ้างอิงส่วนที่ Infineon Technologies ได้ทดสอบไปกับ CSA certified lab tests ในประเทศสิงค์โปร์ โดยที่ Platform Security Functions (PSF) ที่ถูกขีดเส้นใต้ได้รับรองรับ CLS Ready แล้ว PSFs ที่เป็นตัวหนากำลังอยู่ระหว่างของการรับรองในปี 2569

TESA ได้ทำการทดสอบ PSF ที่ได้การรับรองและระหว่างกำลังอยู่ระหว่างของการรับรองเพื่อให้ TESA AIoT Foundation Platform พร้อมรับ CLS Ready ให้เร็วที่สุด นอกจากนั้น TESA ได้ทำงานร่วมกับ Infineon Technologies และ test labs เพื่อให้เข้าใจกระบวนการและวิธีการทดสอบเพื่อให้ TESA AIoT Foundation Platform พร้อมสำหรับเป็นจุดต่อยอดให้กับนักพัฒนาไทยที่อยากพัฒนาผลิตภัณฑ์ได้อย่างรวดเร็ว มีความปลอดภัย ได้มาตราฐาน และสามารถขายไปทั่วโลกได้

<table>
  <tr>
    <td><b>Attestation</b></td>
    <td><u>Root-of-Trust</u></td>
    <td><u>Secure Communication to External Entities</u></td>
    <td><u>Secure Key Storage</u></td>
  </tr>
  <tr>
    <td><b>Debug Access Port</b></td>
    <td><u>Secure Access Policy</u></td>
    <td><u>Secure Data Storage</u></td>
    <td><u>Secure Key Update</u></td>
  </tr>
  <tr>
    <td>Memory Protection</td>
    <td><u>Secure binding of MCU with SE</u></td>
    <td><u>Secured Device Identity</u></td>
    <td>Secure Reset</td>
  </tr>
  <tr>
    <td><u>Permanent Decommissioning</u></td>
    <td><b>Secure Boot</b></td>
    <td><b>Secure Firmware Update</b></td>
    <td><u>Temporary Decommissioning</u></td>
  </tr>
  <tr>
    <td><b>Platform Integrity Verification</b></td>
    <td><u>Secure Bus Communication</u></td>
    <td><u>Secure Key Generation</u></td>
    <td>Others</td>
  </tr>
</table>

## ความสามารถของ TESA AIoT Foundation Platform เทียบกับ Cybersecurity Compliance Controls as of Feb 13, 2026

| TESA AIoT Foundation Platform Features | NCSA IoT Cybersecurity Guideline | CLS Ready (Platform Security Functions) | CLS Level 4 Minimum Test Spec | ETSI EN303 645 |
| :-------------------------------------: | :-----------------------------------------------: | :-------------------------------------: | :-------------------------------------------------------------------------------------------------------------------------------------------------------: | :----------------: |
| API to store secrets on secure element | Secure secret storage \[6.4] | Secure Key Storage | - Physical Attacks- Side channel analysis and fault injection | 5.4-1 |
| API to generate passwords | Password management (password generation) \[6.1] | Secure Key Generation | - Physical Attacks- Side channel analysis and fault injection | 5.4-4 |
| API to reset passwords | Password management (password reset) \[6.1] | Secure Key Update | Physical Attacks | |
| Platform relying on root of trust and certificate chain | | Root-of-Trust | - Physical Attacks- Side channel analysis and fault injection | 5.4-2,5.7-1 |
| Device bootup using secrets from secure element | - Secure secret storage \[6.4]- Secure communication \[6.5] | Secure Boot | - Side channel analysis and fault injection | 5.6-3,5.7-1 |
| API to securely store device identity | | Secured Device Identity | | 5.4-2 |
| Support attestation | | Attestation | | |
| - End to end communication from device to platform- Plaform verifing device identity | - Personal information protection \[6.7]- Input validation \[6.9] | Platform Integrity Verification | - Physical Attacks- Side channel analysis and fault injection | 5.4-2 |
| Secure over the air update | - Secure software update \[6.3]- Secure communication \[6.5] | Secure Firmware Update | Tests related to FW update | 5.3-7,5.3-9,5.3-10 |
| Encrption at rest for storing personal information | Personal information protection \[6.7] | Secure Data Storage | - Physical Attacks- Side channel analysis and fault injection | 5.4-1 |
| End to end communication from device to platform | Personal information protection \[6.7] | Secure Access Policy | | 5.6-8 |
| | | Memory Protection | | 5.6-8 |
| API to manage debug access port security | Attack surface reduction \[6.6] | Debug Access Port Security | - Tests related to FW retrieval from debuggingports- Physical Attacks to ensure that the device does not have unnecessary exposed physical interfaces | 5.6-3.5.6-8 |
| | | Secure Reset | | 5.11-1 |
| API to temporary decommision | | Temporary Decommissioning | | 5.11-1 |
| API to permanent decommision | | Permanent Decommissioning | | 5.11-1 |
| End to end communication from device to platform | - Secure communication \[6.5]- Personal information protection \[6.7] | Secure Communication to external entities (e.g., cloud) | Communications, Ports and Services | 5.5-1,5.5-6,5.5-7 |
| API to provide secure bus communication between device and its peripherals | Attack surface reduction \[6.6] | Secure bus communication | - Physical Attacks- Side channel analysis and fault injection | 5.5-6,5.5-7,5.6-3 |
| | | Secure binding of MCU with SE | - Physical Attacks- Side channel analysis and fault injection | 5.5-8 |
| | - Vulnerability disclosure \[6.2]- Regular and proper vulnerability disclosure publications \[6.18] | | | |
| Support platform stored personal information removal | Personal information removal \[6.8] | | | |
| Platform APIs validating user inputs | Input validation \[6.9] | | | |
| | Personal information protection policies \[6.10] | | | |
| Platform built with secure by design approach | Threat modeling \[6.11] | | | |
| Platform built with secure by design approach | Secure engineering approach \[6.12] | | | |
| Platform version releases including secure updates | Secure update announcement \[6.13] | | | |
| Regularly update platform services | Device and related componet security updates \[6.14] | | | |
| Not applicable | Device hardening \[6.15] | | | |
| | Device and related component bill of materials \[6.16] | | | |
| | Penetration testing, vulnerability assessment, and major security updates \[6.17] | | | |
| | Prepare and provide compliance evidence \[6.19] | | | |
| Part of CLS ready certification | 3rd party testing \[6.19 - 6.26] | | | |

//    Copyright (C) 2020 Jakub Melka
//
//    This file is part of PdfForQt.
//
//    PdfForQt is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    PdfForQt is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public License
//    along with PDFForQt. If not, see <https://www.gnu.org/licenses/>.

#ifndef PDFDOCUMENTBUILDER_H
#define PDFDOCUMENTBUILDER_H

#include "pdfobject.h"
#include "pdfdocument.h"
#include "pdfannotation.h"

class QPdfWriter;

namespace pdf
{

using PDFIntegerVector = std::vector<PDFInteger>;

struct WrapName
{
    WrapName(const char* name) :
        name(name)
    {

    }

    QByteArray name;
};

struct WrapAnnotationColor
{
    WrapAnnotationColor(QColor color) :
        color(color)
    {

    }

    QColor color;
};

struct WrapFreeTextAlignment
{
    constexpr inline WrapFreeTextAlignment(Qt::Alignment alignment) :
        alignment(alignment)
    {

    }

    Qt::Alignment alignment;
};

struct WrapString
{
    WrapString(const char* string) :
        string(string)
    {

    }

    QByteArray string;
};

struct WrapCurrentDateTime { };
struct WrapEmptyArray { };

/// Factory for creating various PDF objects, such as simple objects,
/// dictionaries, arrays etc.
class PDFObjectFactory
{
public:
    inline explicit PDFObjectFactory() = default;

    void beginArray();
    void endArray();

    void beginDictionary();
    void endDictionary();

    void beginDictionaryItem(const QByteArray& name);
    void endDictionaryItem();

    PDFObjectFactory& operator<<(std::nullptr_t);
    PDFObjectFactory& operator<<(bool value);
    PDFObjectFactory& operator<<(PDFReal value);
    PDFObjectFactory& operator<<(PDFInteger value);
    PDFObjectFactory& operator<<(PDFObjectReference value);
    PDFObjectFactory& operator<<(WrapName wrapName);
    PDFObjectFactory& operator<<(int value);
    PDFObjectFactory& operator<<(const QRectF& value);
    PDFObjectFactory& operator<<(WrapCurrentDateTime);
    PDFObjectFactory& operator<<(WrapAnnotationColor color);
    PDFObjectFactory& operator<<(QString textString);
    PDFObjectFactory& operator<<(WrapEmptyArray);
    PDFObjectFactory& operator<<(TextAnnotationIcon icon);
    PDFObjectFactory& operator<<(LinkHighlightMode mode);
    PDFObjectFactory& operator<<(WrapFreeTextAlignment alignment);
    PDFObjectFactory& operator<<(WrapString string);
    PDFObjectFactory& operator<<(AnnotationLineEnding lineEnding);
    PDFObjectFactory& operator<<(const QPointF& point);
    PDFObjectFactory& operator<<(const QDateTime& dateTime);
    PDFObjectFactory& operator<<(AnnotationBorderStyle style);
    PDFObjectFactory& operator<<(const PDFObject& object);
    PDFObjectFactory& operator<<(Stamp stamp);
    PDFObjectFactory& operator<<(FileAttachmentIcon icon);

    /// Treat containers - write them as array
    template<typename Container, typename ValueType = decltype(*std::begin(std::declval<Container>()))>
    PDFObjectFactory& operator<<(Container container)
    {
        beginArray();

        auto it = std::begin(container);
        auto itEnd = std::end(container);
        for (; it != itEnd; ++it)
        {
            *this << *it;
        }

        endArray();

        return *this;
    }

    PDFObject takeObject();

    /// Creates text string object from QString, using PDFDocEncoding, if possible,
    /// if not, then UTF-16 BE encoding is used
    /// \param textString Text to be converted
    static PDFObject createTextString(QString textString);
    
private:
    void addObject(PDFObject object);

    enum class ItemType
    {
        Object,
        Dictionary,
        DictionaryItem,
        Array
    };

    /// What is stored in this structure, depends on the type.
    /// If type is 'Object', then single simple object is in object,
    /// if type is dictionary, then PDFDictionary is stored in object,
    /// if type is dictionary item, then object and item name is stored
    /// in the data, if item is array, then array is stored in the data.
    struct Item
    {
        inline Item() = default;

        template<typename T>
        inline Item(ItemType type, T&& data) :
            type(type),
            object(qMove(data))
        {

        }

        template<typename T>
        inline Item(ItemType type, const QByteArray& itemName, T&& data) :
            type(type),
            itemName(qMove(itemName)),
            object(qMove(data))
        {

        }

        ItemType type = ItemType::Object;
        QByteArray itemName;
        std::variant<PDFObject, PDFArray, PDFDictionary> object;
    };

    std::vector<Item> m_items;
};

/// This class can create content streams, using the Qt's QPainter
/// to draw graphics elements on it. Content stream can have various
/// resources, which can, if selected be dereferenced, so content
/// stream is encapsulated and doesn't contain references.
class PDFFORQTLIBSHARED_EXPORT PDFContentStreamBuilder
{
public:

    enum class CoordinateSystem
    {
        Qt, ///< Qt's coordinate system, (0, 0) is top left
        PDF ///< PDF's coordinate system, (0, 0) is top right
    };

    /// Create new content stream builder, with given size and given coordinate system.
    explicit PDFContentStreamBuilder(QSizeF size, CoordinateSystem coordinateSystem);
    ~PDFContentStreamBuilder();

    struct ContentStream
    {
        /// Page object, which has been created by this content stream builder
        PDFObjectReference pageObject;

        /// Contents of the created page
        PDFObject contents;

        /// Resources of the created page
        PDFObject resources;

        /// Temporary document, which has been created by this builder
        PDFDocument document;
    };

    /// Starts painting on a new content stream. This function returns painter,
    /// onto which can be graphics drawn. Painter respects selected coordinate system.
    /// Calling begin multiple times, without subsequent calls to end function,
    /// is invalid and can result in undefined behaviour.
    /// \return Painter, onto which can be graphic content drawn
    QPainter* begin();

    /// Finishes painting on a new content stream. This function returns content
    /// stream object, which is associated with this page. This function
    /// must be called with painter, which has been returned with call to begin function.
    /// \param painter Painter, which was returned with function begin()
    ContentStream end(QPainter* painter);

    /// Returns actually used coordinate system
    CoordinateSystem getCoordinateSystem() const { return m_coordinateSystem; }

private:
    QSizeF m_size;
    CoordinateSystem m_coordinateSystem;
    QBuffer* m_buffer;
    QPdfWriter* m_pdfWriter;
    QPainter* m_painter;
};

class PDFFORQTLIBSHARED_EXPORT PDFDocumentBuilder
{
public:
    /// Creates a new blank document (with no pages)
    explicit PDFDocumentBuilder();

    /// Creates a new document as modification of old document
    explicit PDFDocumentBuilder(const PDFDocument* document);

    /// Resets the object to the initial state.
    /// \warning All data are lost
    void reset();

    /// Create a new blank document with no pages. If some document
    /// is edited at call of this function, then it is lost.
    void createDocument();

    /// Builds a new document. This function can throw exceptions,
    /// if document being built was invalid.
    PDFDocument build();

    /// If object is reference, the dereference attempt is performed
    /// and object is returned. If it is not a reference, then self
    /// is returned. If dereference attempt fails, then null object
    /// is returned (no exception is thrown).
    const PDFObject& getObject(const PDFObject& object) const;

    /// Returns dictionary from an object. If object is not a dictionary,
    /// then nullptr is returned (no exception is thrown).
    const PDFDictionary* getDictionaryFromObject(const PDFObject& object) const;

    /// Returns object by reference. If dereference attempt fails, then null object
    /// is returned (no exception is thrown).
    const PDFObject& getObjectByReference(PDFObjectReference reference) const;

    /// Returns annotation bounding rectangle
    std::array<PDFReal, 4> getAnnotationReductionRectangle(const QRectF& boundingRect, const QRectF& innerRect) const;

    /// If reference \p annotationReference points to supported annotation,
    /// then this function updates annotation's appearance streams.
    /// \param annotationReference Reference to the annotation
    void updateAnnotationAppearanceStreams(PDFObjectReference annotationReference);

    const PDFFormManager* getFormManager() const;
    void setFormManager(const PDFFormManager* formManager);

    /// Flattens page tree, inheritable attributes in non-leaf nodes will
    /// be written into the page tree. Templates will be lost.
    void flattenPageTree();

    /// Sets a list of page references to page tree. Page tree must
    /// be flattened to use this function. \sa flattenPageTree
    /// \param pageReferences Page references
    void setPages(const std::vector<PDFObjectReference>& pageReferences);

    /// Returns a list of page references. Page tree must
    /// be flattened to use this function. \sa flattenPageTree
    std::vector<PDFObjectReference> getPages() const;

    /// Adds a new objet to the object storage
    /// \param object Object
    PDFObjectReference addObject(PDFObject object);

    /// Copies objects from another storage. Objects have adjusted references to match
    /// this storage references and objects are added after the last objects of active storage.
    /// This function can also automatically create references from direct objects passed
    /// by parameter \p objects, if \p createReferences is set to true.
    /// \param objects Objects, which we want to copy from another storage
    /// \param storage Storage, from which we are copying from
    /// \param createReferences Create references from \p objects
    std::vector<PDFObject> copyFrom(const std::vector<PDFObject>& objects, const PDFObjectStorage& storage, bool createReferences);

    /// Creates object list from reference list (objects are references)
    /// \param references References
    static std::vector<PDFObject> createObjectsFromReferences(const std::vector<PDFObjectReference>& references);

    /// Creates reference list from object list. Object list must be
    /// a list of references, it is invalid to pass this function objects,
    /// which are not references.
    /// \param objects Objects with references
    static std::vector<PDFObjectReference> createReferencesFromObjects(const std::vector<PDFObject>& objects);

    /// Returns catalog reference
    PDFObjectReference getCatalogReference() const;

    /// Returns object storage
    const PDFObjectStorage* getStorage() const { return &m_storage; }

    /// Appends object to target object. Targed object reference must be valid.
    /// Arrays are concatenated.
    /// \param reference Target object reference
    /// \param object Object to be appended
    void appendTo(PDFObjectReference reference, PDFObject object);

    /// Merges object to target object. Targed object reference must be valid.
    /// Arrays are not concatenated.
    /// \param reference Target object reference
    /// \param object Object to be merged
    void mergeTo(PDFObjectReference reference, PDFObject object);

    /// Sets object by reference
    /// \param reference Target object reference
    /// \param object Object to be set
    void setObject(PDFObjectReference reference, PDFObject object);

    /// Creates document parts from given pages. Pages must be flattened
    /// by function \p flattenPageTree. \sa flattenPageTree. Each document
    /// part has certain page size, sum of \p parts must equal to page count.
    /// \param parts Parts (page count of each document part)
    void createDocumentParts(const std::vector<size_t>& parts);

/* START GENERATED CODE */

    /// Appends a new page after last page.
    /// \param mediaBox Media box of the page (size of paper)
    PDFObjectReference appendPage(QRectF mediaBox);


    /// Creates URI action.
    /// \param URL Target URL
    PDFObjectReference createActionURI(QString URL);


    /// Caret annotations are used to indicate, where text should be inserted (for example, if reviewer 
    /// reviews the document, and he wants to mark, that some text should be inserted, he uses this 
    /// annotation).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is caret displayed
    /// \param borderWidth Border width
    /// \param color Caret color. If you do not want to have a border, then use invalid QColor.
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationCaret(PDFObjectReference page,
                                             QRectF rectangle,
                                             PDFReal borderWidth,
                                             QColor color,
                                             QString title,
                                             QString subject,
                                             QString contents);


    /// Circle annotation displays ellipse (or circle). Circle border/fill color can be defined, along with 
    /// border width. Popup annotation can be attached to this annotation.
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is circle/ellipse displayed
    /// \param borderWidth Width of the border line of circle/ellipse
    /// \param fillColor Fill color of rectangle (interior color). If you do not want to have area color filled, 
    ///        then use invalid QColor.
    /// \param strokeColor Stroke color (color of the rectangle border). If you do not want to have a 
    ///        border, then use invalid QColor.
    /// \param title Title (it is displayed as title of popup window)
    /// \param subject Subject (short description of the subject being adressed by the annotation)
    /// \param contents Contents (text displayed, for example, in the marked annotation dialog)
    PDFObjectReference createAnnotationCircle(PDFObjectReference page,
                                              QRectF rectangle,
                                              PDFReal borderWidth,
                                              QColor fillColor,
                                              QColor strokeColor,
                                              QString title,
                                              QString subject,
                                              QString contents);


    /// Creates a new file attachment annotation. This annotation needs file specification as parameter.
    /// \param page Page to which is annotation added
    /// \param position Position
    /// \param fileSpecification File specification
    /// \param icon Icon
    /// \param title Title
    /// \param description Description
    PDFObjectReference createAnnotationFileAttachment(PDFObjectReference page,
                                                      QPointF position,
                                                      PDFObjectReference fileSpecification,
                                                      FileAttachmentIcon icon,
                                                      QString title,
                                                      QString description);


    /// Free text annotation displays text directly on a page. Text appears directly on the page, in the 
    /// same way, as standard text in PDF document. Free text annotations are usually used to comment 
    /// the document. Free text annotation can also have callout line, with, or without a knee.
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is text displayed
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents (text displayed)
    /// \param textAlignment Text alignment. Only horizontal alignment flags are valid.
    PDFObjectReference createAnnotationFreeText(PDFObjectReference page,
                                                QRectF rectangle,
                                                QString title,
                                                QString subject,
                                                QString contents,
                                                TextAlignment textAlignment);


    /// Free text annotation displays text directly on a page. Text appears directly on the page, in the 
    /// same way, as standard text in PDF document. Free text annotations are usually used to comment 
    /// the document. Free text annotation can also have callout line, with, or without a knee. Specify 
    /// start/end point parameters of this function to get callout line.
    /// \param page Page to which is annotation added
    /// \param boundingRectangle Bounding rectangle of free text annotation. It must contain both 
    ///        callout line and text rectangle.
    /// \param textRectangle Rectangle with text, in absolute coordinates. They are then recomputed to 
    ///        match bounding rectangle.
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents (text displayed)
    /// \param textAlignment Text alignment. Only horizontal alignment flags are valid.
    /// \param startPoint Start point of callout line
    /// \param endPoint End point of callout line
    /// \param startLineType Line ending at the start point
    /// \param endLineType Line ending at the end point
    PDFObjectReference createAnnotationFreeText(PDFObjectReference page,
                                                QRectF boundingRectangle,
                                                QRectF textRectangle,
                                                QString title,
                                                QString subject,
                                                QString contents,
                                                TextAlignment textAlignment,
                                                QPointF startPoint,
                                                QPointF endPoint,
                                                AnnotationLineEnding startLineType,
                                                AnnotationLineEnding endLineType);


    /// Free text annotation displays text directly on a page. Text appears directly on the page, in the 
    /// same way, as standard text in PDF document. Free text annotations are usually used to comment 
    /// the document. Free text annotation can also have callout line, with, or without a knee. Specify 
    /// start/end point parameters of this function to get callout line.
    /// \param page Page to which is annotation added
    /// \param boundingRectangle Bounding rectangle of free text annotation. It must contain both 
    ///        callout line and text rectangle.
    /// \param textRectangle Rectangle with text, in absolute coordinates. They are then recomputed to 
    ///        match bounding rectangle.
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents (text displayed)
    /// \param textAlignment Text alignment. Only horizontal alignment flags are valid.
    /// \param startPoint Start point of callout line
    /// \param kneePoint Knee point of callout line
    /// \param endPoint End point of callout line
    /// \param startLineType Line ending at the start point
    /// \param endLineType Line ending at the end point
    PDFObjectReference createAnnotationFreeText(PDFObjectReference page,
                                                QRectF boundingRectangle,
                                                QRectF textRectangle,
                                                QString title,
                                                QString subject,
                                                QString contents,
                                                TextAlignment textAlignment,
                                                QPointF startPoint,
                                                QPointF kneePoint,
                                                QPointF endPoint,
                                                AnnotationLineEnding startLineType,
                                                AnnotationLineEnding endLineType);


    /// Text markup annotation is used to highlight text. It is a markup annotation, so it can contain 
    /// window to be opened (and commented). This annotation is usually used to highlight text, but can 
    /// also highlight other things, such as images, or other graphics.
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is highlight displayed
    /// \param color Color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationHighlight(PDFObjectReference page,
                                                 QRectF rectangle,
                                                 QColor color,
                                                 QString title,
                                                 QString subject,
                                                 QString contents);


    /// Text markup annotation is used to highlight text. It is a markup annotation, so it can contain 
    /// window to be opened (and commented). This annotation is usually used to highlight text, but can 
    /// also highlight other things, such as images, or other graphics.
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is highlight displayed
    /// \param color Color
    PDFObjectReference createAnnotationHighlight(PDFObjectReference page,
                                                 QRectF rectangle,
                                                 QColor color);


    /// Ink anotation represents freehand scribble composed from one or more disjoint paths.
    /// \param page Page to which is annotation added
    /// \param inkPoints Ink points
    /// \param borderWidth Border line width
    /// \param strokeColor Stroke color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationInk(PDFObjectReference page,
                                           QPolygonF inkPoints,
                                           PDFReal borderWidth,
                                           QColor strokeColor,
                                           QString title,
                                           QString subject,
                                           QString contents);


    /// Ink anotation represents freehand scribble composed from one or more disjoint paths.
    /// \param page Page to which is annotation added
    /// \param inkPoints Ink points (vector of polygons)
    /// \param borderWidth Border line width
    /// \param strokeColor Stroke color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationInk(PDFObjectReference page,
                                           Polygons inkPoints,
                                           PDFReal borderWidth,
                                           QColor strokeColor,
                                           QString title,
                                           QString subject,
                                           QString contents);


    /// Line annotation represents straight line, or some more advanced graphics, such as dimension with 
    /// text. Line annotations are markup annotations, so they can have popup window. Line endings can 
    /// be specified.
    /// \param page Page to which is annotation added
    /// \param boundingRect Line annotation bounding rectangle
    /// \param startPoint Line start
    /// \param endPoint Line end
    /// \param lineWidth Line width
    /// \param fillColor Fill color of line parts (for example, filled line endings)
    /// \param strokeColor Line stroke color
    /// \param title Title (it is displayed as title of popup window)
    /// \param subject Subject (short description of the subject being adressed by the annotation)
    /// \param contents Contents (text displayed, for example, in the marked annotation dialog)
    /// \param startLineType Start line ending type
    /// \param endLineType End line ending type
    PDFObjectReference createAnnotationLine(PDFObjectReference page,
                                            QRectF boundingRect,
                                            QPointF startPoint,
                                            QPointF endPoint,
                                            PDFReal lineWidth,
                                            QColor fillColor,
                                            QColor strokeColor,
                                            QString title,
                                            QString subject,
                                            QString contents,
                                            AnnotationLineEnding startLineType,
                                            AnnotationLineEnding endLineType);


    /// Line annotation represents straight line, or some more advanced graphics, such as dimension with 
    /// text. Line annotations are markup annotations, so they can have popup window. Line endings can 
    /// be specified.
    /// \param page Page to which is annotation added
    /// \param boundingRect Line annotation bounding rectangle
    /// \param startPoint Line start
    /// \param endPoint Line end
    /// \param lineWidth Line width
    /// \param fillColor Fill color of line parts (for example, filled line endings)
    /// \param strokeColor Line stroke color
    /// \param title Title (it is displayed as title of popup window)
    /// \param subject Subject (short description of the subject being adressed by the annotation)
    /// \param contents Contents (text displayed, for example, in the marked annotation dialog)
    /// \param startLineType Start line ending type
    /// \param endLineType End line ending type
    /// \param leaderLineLength Length of the leader line. Leader line extends from each endpoint of 
    ///        the line perpendicular to the line itself. Value can be either positive, negative or zero. If 
    ///        positive, then extension is in plane that is above the annotation line (in clockwise order), 
    ///        if negative, then it is below the annotation line.
    /// \param leaderLineOffset Length of leader line offset, which is the amount of empty space 
    ///        between the endpoints of the annotation and beginning of leader lines
    /// \param leaderLineExtension Length of leader line extension, which extends leader lines in 180° 
    ///        direction from leader lines (so leader lines continues above drawn line)
    /// \param displayContents Display contents of the annotation as text along the line
    /// \param displayedContentsTopAlign Displayed contents appear above the line, instead inline.
    PDFObjectReference createAnnotationLine(PDFObjectReference page,
                                            QRectF boundingRect,
                                            QPointF startPoint,
                                            QPointF endPoint,
                                            PDFReal lineWidth,
                                            QColor fillColor,
                                            QColor strokeColor,
                                            QString title,
                                            QString subject,
                                            QString contents,
                                            AnnotationLineEnding startLineType,
                                            AnnotationLineEnding endLineType,
                                            PDFReal leaderLineLength,
                                            PDFReal leaderLineOffset,
                                            PDFReal leaderLineExtension,
                                            bool displayContents,
                                            bool displayedContentsTopAlign);


    /// Creates new link annotation. It usually represents clickable hypertext link. User can also specify 
    /// action, which can be executed, for example, link can be also in the PDF document (link to some 
    /// location in document).
    /// \param page Page to which is annotation added
    /// \param linkRectangle Link rectangle
    /// \param action Action to be performed when user clicks on a link
    /// \param highlightMode Highlight mode
    PDFObjectReference createAnnotationLink(PDFObjectReference page,
                                            QRectF linkRectangle,
                                            PDFObjectReference action,
                                            LinkHighlightMode highlightMode);


    /// Creates new link annotation. It usually represents clickable hypertext link. User can also specify 
    /// action, which can be executed, for example, link can be also in the PDF document (link to some 
    /// location in document).
    /// \param page Page to which is annotation added
    /// \param linkRectangle Link rectangle
    /// \param URL URL to be launched when user clicks on the link
    /// \param highlightMode Highlight mode
    PDFObjectReference createAnnotationLink(PDFObjectReference page,
                                            QRectF linkRectangle,
                                            QString URL,
                                            LinkHighlightMode highlightMode);


    /// Polygon annotation. When opened, they display pop-up window containing the text of associated 
    /// note (and window title), if popup annotation is attached. Polygon border/fill color can be defined, 
    /// along with border width.
    /// \param page Page to which is annotation added
    /// \param polygon Polygon
    /// \param borderWidth Border line width
    /// \param fillColor Fill color
    /// \param strokeColor Stroke color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationPolygon(PDFObjectReference page,
                                               QPolygonF polygon,
                                               PDFReal borderWidth,
                                               QColor fillColor,
                                               QColor strokeColor,
                                               QString title,
                                               QString subject,
                                               QString contents);


    /// Polyline annotation. When opened, they display pop-up window containing the text of associated 
    /// note (and window title), if popup annotation is attached. Polyline border/fill color can be defined, 
    /// along with border width.
    /// \param page Page to which is annotation added
    /// \param polyline Polyline
    /// \param borderWidth Border line width
    /// \param fillColor Fill color
    /// \param strokeColor Stroke color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    /// \param startLineType Start line ending type
    /// \param endLineType End line ending type
    PDFObjectReference createAnnotationPolyline(PDFObjectReference page,
                                                QPolygonF polyline,
                                                PDFReal borderWidth,
                                                QColor fillColor,
                                                QColor strokeColor,
                                                QString title,
                                                QString subject,
                                                QString contents,
                                                AnnotationLineEnding startLineType,
                                                AnnotationLineEnding endLineType);


    /// Creates a new popup annotation on the page. Popup annotation is represented usually by floating 
    /// window, which can be opened, or closed. Popup annotation is associated with parent annotation, 
    /// which can be usually markup annotation. Popup annotation displays parent annotation's texts, for 
    /// example, title, comment, date etc.
    /// \param page Page to which is annotation added
    /// \param parentAnnotation Parent annotation (for which is popup window displayed)
    /// \param rectangle Area on the page, where popup window appears
    /// \param opened Is the window opened?
    PDFObjectReference createAnnotationPopup(PDFObjectReference page,
                                             PDFObjectReference parentAnnotation,
                                             QRectF rectangle,
                                             bool opened);


    /// Square annotation displays rectangle (or square). When opened, they display pop-up window 
    /// containing the text of associated note (and window title), if popup annotation is attached. Square 
    /// border/fill color can be defined, along with border width.
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is rectangle displayed
    /// \param borderWidth Width of the border line of rectangle
    /// \param fillColor Fill color of rectangle (interior color). If you do not want to have area color filled, 
    ///        then use invalid QColor.
    /// \param strokeColor Stroke color (color of the rectangle border). If you do not want to have a 
    ///        border, then use invalid QColor.
    /// \param title Title (it is displayed as title of popup window)
    /// \param subject Subject (short description of the subject being adressed by the annotation)
    /// \param contents Contents (text displayed, for example, in the marked annotation dialog)
    PDFObjectReference createAnnotationSquare(PDFObjectReference page,
                                              QRectF rectangle,
                                              PDFReal borderWidth,
                                              QColor fillColor,
                                              QColor strokeColor,
                                              QString title,
                                              QString subject,
                                              QString contents);


    /// Text markup annotation is used to squiggly underline text. It is a markup annotation, so it can 
    /// contain window to be opened (and commented).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is markup displayed
    /// \param color Color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationSquiggly(PDFObjectReference page,
                                                QRectF rectangle,
                                                QColor color,
                                                QString title,
                                                QString subject,
                                                QString contents);


    /// Text markup annotation is used to squiggly underline text. It is a markup annotation, so it can 
    /// contain window to be opened (and commented).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is markup displayed
    /// \param color Color
    PDFObjectReference createAnnotationSquiggly(PDFObjectReference page,
                                                QRectF rectangle,
                                                QColor color);


    /// Stamp annotation
    /// \param page Page to which is annotation added
    /// \param rectangle Stamp area
    /// \param stampType Stamp type
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationStamp(PDFObjectReference page,
                                             QRectF rectangle,
                                             Stamp stampType,
                                             QString title,
                                             QString subject,
                                             QString contents);


    /// Text markup annotation is used to strikeout text. It is a markup annotation, so it can contain 
    /// window to be opened (and commented).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is markup displayed
    /// \param color Color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationStrikeout(PDFObjectReference page,
                                                 QRectF rectangle,
                                                 QColor color,
                                                 QString title,
                                                 QString subject,
                                                 QString contents);


    /// Text markup annotation is used to strikeout text. It is a markup annotation, so it can contain 
    /// window to be opened (and commented).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is markup displayed
    /// \param color Color
    PDFObjectReference createAnnotationStrikeout(PDFObjectReference page,
                                                 QRectF rectangle,
                                                 QColor color);


    /// Creates text annotation. Text annotation is "sticky note" attached to a point in the PDF document. 
    /// When closed, it is displayed as icon, if opened, widget appears with attached text. Text annotations 
    /// do not scale or rotate, they appear independent of zoom/rotate. So, they behave as if flags 
    /// NoZoom or NoRotate to the annotations are being set. Popup annotation is automatically created 
    /// for this annotation.
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is icon displayed
    /// \param iconType Icon type
    /// \param title Title (it is displayed as title of popup window)
    /// \param subject Subject (short description of the subject being adressed by the annotation)
    /// \param contents Contents (text displayed, for example, in the marked annotation dialog)
    /// \param open Is annotation initially displayed as opened?
    PDFObjectReference createAnnotationText(PDFObjectReference page,
                                            QRectF rectangle,
                                            TextAnnotationIcon iconType,
                                            QString title,
                                            QString subject,
                                            QString contents,
                                            bool open);


    /// Text markup annotation is used to underline text. It is a markup annotation, so it can contain 
    /// window to be opened (and commented).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is markup displayed
    /// \param color Color
    PDFObjectReference createAnnotationUnderline(PDFObjectReference page,
                                                 QRectF rectangle,
                                                 QColor color);


    /// Text markup annotation is used to underline text. It is a markup annotation, so it can contain 
    /// window to be opened (and commented).
    /// \param page Page to which is annotation added
    /// \param rectangle Area in which is markup displayed
    /// \param color Color
    /// \param title Title
    /// \param subject Subject
    /// \param contents Contents
    PDFObjectReference createAnnotationUnderline(PDFObjectReference page,
                                                 QRectF rectangle,
                                                 QColor color,
                                                 QString title,
                                                 QString subject,
                                                 QString contents);


    /// Creates empty catalog. This function is used, when a new document is being created. Do not call 
    /// this function manually.
    PDFObjectReference createCatalog();


    /// Creates page tree root for the catalog. This function is only called when new document is being 
    /// created. Do not call this function manually.
    PDFObjectReference createCatalogPageTreeRoot();


    /// Creates document part item (for certain range of pages).
    /// \param startPage First page of page range.
    /// \param endPage Last page of page range.
    /// \param parent Parent node (must be a reference to parent node).
    PDFObjectReference createDocumentPartItem(PDFObjectReference startPage,
                                              PDFObjectReference endPage,
                                              PDFObjectReference parent);


    /// Creates document part root node (and setups catalog object).
    PDFObjectReference createDocumentPartRoot();


    /// This function is used to create a new trailer dictionary, when blank document is created. Do not 
    /// call this function manually.
    /// \param catalog Reference to document catalog
    PDFObject createTrailerDictionary(PDFObjectReference catalog);


    /// Removes document actions from document catalog.
    void removeDocumentActions();


    /// Removes outline tree from document catalog.
    void removeOutline();


    /// Removes structure tree from document catalog.
    void removeStructureTree();


    /// Removes threads from document catalog.
    void removeThreads();


    /// Sets annotation appearance state.
    /// \param annotation Annotation
    /// \param appearanceState Appearance state
    void setAnnotationAppearanceState(PDFObjectReference annotation,
                                      QByteArray appearanceState);


    /// Sets annotation border.
    /// \param annotation Annotation
    /// \param hRadius Horizontal corner radius
    /// \param vRadius Vertical corner radius
    /// \param width Line width
    void setAnnotationBorder(PDFObjectReference annotation,
                             PDFReal hRadius,
                             PDFReal vRadius,
                             PDFReal width);


    /// Sets annotation border style.
    /// \param annotation Annotation
    /// \param style Style
    /// \param width Width
    void setAnnotationBorderStyle(PDFObjectReference annotation,
                                  AnnotationBorderStyle style,
                                  PDFReal width);


    /// Sets annotation color.
    /// \param annotation Annotation
    /// \param color Color
    void setAnnotationColor(PDFObjectReference annotation,
                            QColor color);


    /// Sets annotation contents.
    /// \param annotation Annotation
    /// \param contents Contents
    void setAnnotationContents(PDFObjectReference annotation,
                               QString contents);


    /// Sets constant opacity of annotation's graphics.
    /// \param annotation Annotation
    /// \param opacity Opacity (value must be in range from 0.0 to 1.0)
    void setAnnotationOpacity(PDFObjectReference annotation,
                              PDFReal opacity);


    /// Sets open state of the annotation.
    /// \param annotation Annotation
    /// \param isOpen Is annotation opened?
    void setAnnotationOpenState(PDFObjectReference annotation,
                                bool isOpen);


    /// Sets annotation quadrilaterals. Quadrilaterals are sequence of 4 points, where first two points are 
    /// on the upper side of quadrilateral, and the last two points are on the lower side of quadrilateral. 
    /// Quadrilaterals are represented as unclosed polygon with 4 * n vertices.
    /// \param annotation Annotation
    /// \param quadrilaterals Quadrilaterals
    void setAnnotationQuadPoints(PDFObjectReference annotation,
                                 QPolygonF quadrilaterals);


    /// Sets annotation rich text contents. This function will work only on markup annotations.
    /// \param annotation Annotation
    /// \param richText Rich text contents
    void setAnnotationRichText(PDFObjectReference annotation,
                               QString richText);


    /// Sets annotation subject.
    /// \param annotation Annotation
    /// \param subject Subject
    void setAnnotationSubject(PDFObjectReference annotation,
                              QString subject);


    /// Sets annotation title.
    /// \param annotation Annotation
    /// \param title Title
    void setAnnotationTitle(PDFObjectReference annotation,
                            QString title);


    /// Set AcroForm to catalog.
    /// \param acroForm Reference to AcroForm object.
    void setCatalogAcroForm(PDFObjectReference acroForm);


    /// Set optional content properties to catalog.
    /// \param ocProperties Reference to catalog optional content properties.
    void setCatalogOptionalContentProperties(PDFObjectReference ocProperties);


    /// Set document author.
    /// \param author Author
    void setDocumentAuthor(QString author);


    /// Set document creation date.
    /// \param creationDate Creation date/time
    void setDocumentCreationDate(QDateTime creationDate);


    /// Set document creator.
    /// \param creator Creator
    void setDocumentCreator(QString creator);


    /// Set document keywords.
    /// \param keywords Keywords
    void setDocumentKeywords(QString keywords);


    /// Set document producer.
    /// \param producer Producer
    void setDocumentProducer(QString producer);


    /// Set document subject.
    /// \param subject Subject
    void setDocumentSubject(QString subject);


    /// Set document title.
    /// \param title Title
    void setDocumentTitle(QString title);


    /// Sets form field list box indices.
    /// \param formField Form field
    /// \param indices Sorted list of selected indices
    void setFormFieldChoiceIndices(PDFObjectReference formField,
                                   PDFIntegerVector indices);


    /// Sets form field list box top index. Top index is zero-based index of first item visible in the list box.
    /// \param formField Form field
    /// \param topIndex Zero-based index of first visible item
    void setFormFieldChoiceTopIndex(PDFObjectReference formField,
                                    PDFInteger topIndex);


    /// Sets form field value. Value must be correct for this form field, no checking is performed. Also, if 
    /// you use this function, annotation widgets, which are attached to this form field, should also be 
    /// updated (for example, appearance state and sometimes appearance streams).
    /// \param formField Form field
    /// \param value Value
    void setFormFieldValue(PDFObjectReference formField,
                           PDFObject value);


    /// Set document language.
    /// \param locale Locale, from which is language determined
    void setLanguage(QLocale locale);


    /// Set document language.
    /// \param language Document language. It should be a language identifier, as defined in ISO 639 
    ///        and ISO 3166. For example, "en-US", where first two letter means language code (en = 
    ///        english), and the latter two is country code (US - United States).
    void setLanguage(QString language);


    /// Sets document part to page.
    /// \param page Page
    /// \param documentPart Document part
    void setPageDocumentPart(PDFObjectReference page,
                             PDFObjectReference documentPart);


    /// This function is used to update trailer dictionary. Must be called each time the final document is 
    /// being built.
    /// \param objectCount Number of objects (including empty ones)
    void updateTrailerDictionary(PDFInteger objectCount);


/* END GENERATED CODE */

private:
    QRectF getPopupWindowRect(const QRectF& rectangle) const;
    QString getProducerString() const;
    PDFObjectReference getPageTreeRoot() const;
    PDFInteger getPageTreeRootChildCount() const;
    PDFObjectReference getDocumentInfo() const;
    void updateDocumentInfo(PDFObject info);
    QRectF getPolygonsBoundingRect(const Polygons& Polygons) const;

    PDFObjectStorage m_storage;
    PDFVersion m_version;
    const PDFFormManager* m_formManager = nullptr;
};

/// This class serves for document modification. While document is modified,
/// modification flags are gathered. At the end of the modification, it is checked,
/// if document was really changed.
class PDFFORQTLIBSHARED_EXPORT PDFDocumentModifier
{
public:
    explicit PDFDocumentModifier(const PDFDocument* originalDocument);

    /// Returns builder, which can modify document
    PDFDocumentBuilder* getBuilder() { return &m_builder; }

    /// Finalizes document modification and prepares new changed document.
    /// If document content is equal to the original, then false is returned,
    /// otherwise true is returned. If document was not modified,
    /// then new document is not created and function \p getDocument
    /// will return nullptr.
    bool finalize();

    PDFDocumentPointer getDocument() const { return m_modifiedDocument; }
    PDFModifiedDocument::ModificationFlags getFlags() const { return m_modificationFlags; }

    void markReset() { m_modificationFlags.setFlag(PDFModifiedDocument::Reset); }
    void markAnnotationsChanged() { m_modificationFlags.setFlag(PDFModifiedDocument::Annotation); }
    void markFormFieldChanged() { m_modificationFlags.setFlag(PDFModifiedDocument::FormField); }

private:
    const PDFDocument* m_originalDocument;
    PDFDocumentBuilder m_builder;
    PDFDocumentPointer m_modifiedDocument;
    PDFModifiedDocument::ModificationFlags m_modificationFlags;
};

// Implementation

inline
const PDFObject& PDFDocumentBuilder::getObject(const PDFObject& object) const
{
    if (object.isReference())
    {
        // Try to dereference the object
        return m_storage.getObject(object.getReference());
    }

    return object;
}

inline
const PDFDictionary* PDFDocumentBuilder::getDictionaryFromObject(const PDFObject& object) const
{
    const PDFObject& dereferencedObject = getObject(object);
    if (dereferencedObject.isDictionary())
    {
        return dereferencedObject.getDictionary();
    }
    else if (dereferencedObject.isStream())
    {
        return dereferencedObject.getStream()->getDictionary();
    }

    return nullptr;
}

inline
const PDFObject& PDFDocumentBuilder::getObjectByReference(PDFObjectReference reference) const
{
    return m_storage.getObject(reference);
}

}   // namespace pdf

#endif // PDFDOCUMENTBUILDER_H
